#pragma once

#include <iostream>
#include <memory>
#include <atomic>
#include <cstdint>
#include <map>
#include <new>
#include <limits>
#include <algorithm>
#include <cassert>
#include <Windows.h>
#include <intrin.h>

#include "util/common_util.h"
#include "logging/logger.h"

namespace win
{
	extern std::atomic<std::uint32_t> num_of_free_object_pool;
	extern void* pool_table[];

	enum ObjectPoolErrorCode
	{
		SUCCESS,
		RESERVE_ERROR,
		COMMIT_ERROR,
		EXCEED_MAX_CAPACITY_ERROR,
	};

	constexpr int INVALID_OBJECT = -1;

	// ObjectPool
	// - 미리 할당된 객체를 memory-safe 하게 반환받을 수 있는 클래스.
	// 
	// - 성능
	// 1. 메모리 할당/해제 오버헤드 제거
	// 2. locality 증가
	//	  - 모든 객체가 연속된 가상 주소 공간에 할당되어 노드 기반의 map 자료구조보다 fragmentation이 적음. (WSS 감소)
	//    - (?) 순차적 순회로 cache hit 비율 증가
	// 3. 메모리 효율성
	//	  - VirtualAlloc으로 주소 공간만 예약해두기 때문에 물리 메모리 프레임 낭비를 최소화한다.
	// 4. (TODO) 메모리 확장 시 발생하는 오버헤드 측정 필요
	//    - VirtualAlloc으로 커밋 vs std::deque처럼 청크 구조 구현 (현재는 VirtualAlloc에 의존)
	// 
	// - 안정성
	// 1. memory-safey
	//	  - 객체(object)는 unique_ptr 방식으로 동작하는 SafePointer 클래스로 랩핑하여 메모리 안정성 보장한다.
	//		- (Decision) 약간의 메모리 오버헤드가 있지만 메모리 오버헤드는 감수한다.
	//	  - ObjectPool이 객체보다 먼저 해제되지 않된다.
	//		- (Decision) 대부분의 ObjectPool은 프로그램 종료까지 유지하므로 리스크가 있음에도 유지한다.
	// 2. thread-safe
	//    - 기본적으로 클래스가 thread-safe하게 설계되지 않음.
	//		- 아직까진 ObjectPool을 여러 스레드로 공유할 일이 없어 문제가 되지 않는다. 
	//	  - 객체의 할당은 한 스레드에서 수행되지만, 반환은 다른 스레드 흐름에서 수행될 수 있어 조치가 필요하다.
	//		- bitmap search로 해결
	//		  - 탐색이 O(n) 시간을 갖지만, n이 보통 10000 정도라서 오히려 다른 방법(lock, lock-free 등)에 비해 빠를 거라 예상
	//        - CPU 캐시의 도움으로 RAM 모델에서 예측한 시간보다 향상된 성능을 보여줄 거라 예상
	//		  - (TODO) 성능 측정 필요
	//    - (TODO) 나머지 thrad-safe 처리는 필요에 따라 추가 예정.

	template <typename T>
	class ObjectPool : util::NonCopyable, util::NonMovable
	{
	public:
		using object_type = T;
		using size_type = std::size_t;
		using index_type = std::uint32_t;
		using object_pointer = T*;

		static constexpr int MEMORY_PROTECTION_LEVEL = PAGE_READWRITE;
		static constexpr int SIZE_TYPE_BIT_SIZE = sizeof(size_type) * 8;
		static constexpr size_type DEFAULT_CAPACITY = 512;

		class ObjectKey : util::NonCopyable {
			static constexpr auto INVALID_OBJECT_KEY = 0;
		public:
			ObjectKey() noexcept
				: compound_key{ INVALID_OBJECT_KEY  }
			{ }

			ObjectKey(index_type object_pool_id, index_type object_id)
				: compound_key{ (std::uint64_t(object_pool_id) << 32) | object_id }
			{ 
				assert(object_pool_id != 0);
			}

			~ObjectKey()
			{
				clear();
			}

			ObjectKey(ObjectKey&& other) noexcept
			{
				compound_key = other.compound_key;
				other.compound_key = INVALID_OBJECT_KEY;
			}

			ObjectKey& operator=(ObjectKey&& other) noexcept
			{
				if (this != &other) {
					clear();
					compound_key = other.compound_key;
					other.compound_key = INVALID_OBJECT_KEY;
				}
			}

			object_type& get_object()
			{
				return get_object_pool().get_object(object_id());
			}

			void clear()
			{
				if (is_valid())
					get_object_pool().free_object(object_id());
			}

			inline bool is_valid() const
			{
				return compound_key != INVALID_OBJECT_KEY;
			}

		private:
			inline index_type object_pool_id() const
			{
				return compound_key >> 32;
			}

			inline index_type object_id() const
			{
				return compound_key & 0xFFFFFFFF;
			}

			inline ObjectPool& get_object_pool() const
			{
				return *static_cast<ObjectPool*>(pool_table[object_pool_id()]);
			}

			std::uint64_t compound_key;
		};

		ObjectPool() = delete;

		ObjectPool(size_type max_capacity);

		~ObjectPool();

		bool reserve(size_type new_capacity);

		// new_object returns object's id that must manually free.
		// so it is used only when the user guarantees to free.

		template <typename... Args>
		int new_object(Args&&... args)
		{
			auto object_id = get_free_object_index();
			if (object_id >= m_capacity && not extend_capacity(size_type(object_id) + 1))
				return INVALID_OBJECT;

			auto new_object_storage = &m_storage[object_id];
			new (new_object_storage) T(std::forward<Args>(args)...);
			// manually construct using placement new.
			// new operator may simply returns its second argument unchanged.
			// ref. https://en.cppreference.com/w/cpp/language/new

			return transition_to_ready(object_id);
		}

		template <typename... Args>
		ObjectKey new_object_safe(Args&&... args)
		{
			int object_id = new_object(std::forward<Args>(args)...);
			if (object_id == INVALID_OBJECT)
				return ObjectKey{};

			return ObjectKey(m_pool_id, object_id);
		}

		bool free_object(int object_id)
		{
			return transition_to_free(object_id);
		}

		object_type& get_object(int object_id)
		{
			return m_storage[object_id];
		}

		size_type capacity()
		{
			return m_capacity;
		}

	private:

		inline bool is_exceed_max_capacity() const
		{
			return m_capacity >= m_max_capacity;
		}

		bool extend_capacity(size_type minimum_capacity);

		index_type get_free_object_index() const;

		int transition_to_ready(index_type object_id);

		bool transition_to_free(int object_id);
		
		const index_type m_pool_id;

		object_pointer m_storage;
		size_type m_capacity;
		const size_type m_max_capacity;
		
		const size_type m_bitmap_size;
		std::unique_ptr<size_type[]> m_free_object_bitmaps;
	};
}

/// Definitions

namespace win
{
	template <typename T>
	ObjectPool<T>::ObjectPool(size_type max_capacity)
		: m_pool_id{ num_of_free_object_pool.fetch_sub(1) }
		, m_capacity{ std::min(DEFAULT_CAPACITY, max_capacity) }
		, m_max_capacity{ max_capacity }
		, m_bitmap_size{ 1 + max_capacity / SIZE_TYPE_BIT_SIZE }
		, m_free_object_bitmaps{ new size_type[m_bitmap_size]}
	{
		if (not m_pool_id)
			return;

		// register this pool to the table;
		pool_table[m_pool_id] = this;

		m_storage = static_cast<object_pointer>(
			::VirtualAlloc(NULL, max_capacity * sizeof(T), MEM_RESERVE, MEMORY_PROTECTION_LEVEL)
			);

		if (m_storage == nullptr)
			throw ObjectPoolErrorCode::RESERVE_ERROR;

		if (not ::VirtualAlloc(m_storage, m_capacity * sizeof(T), MEM_COMMIT, MEMORY_PROTECTION_LEVEL))
			throw ObjectPoolErrorCode::COMMIT_ERROR;

		// set bitmaps all object are free.
		for (index_type i = 0; i < m_bitmap_size; i++)
			m_free_object_bitmaps[i] = ~size_type(0);
	}

	template <typename T>
	ObjectPool<T>::~ObjectPool()
	{
		// deregister this pool to the table;
		pool_table[m_pool_id] = nullptr;

		if (m_storage != nullptr) {
			if (not ::VirtualFree(m_storage, 0, MEM_RELEASE))
				logging::cerr() << "VirtualFree() failed with " << ::GetLastError();
		}
	}

	template <typename T>
	bool ObjectPool<T>::reserve(size_type new_capacity)
	{
		if (m_capacity >= new_capacity)
			return true;

		if (is_exceed_max_capacity())
			return false;

		new_capacity = std::min(new_capacity, m_max_capacity);
		if (::VirtualAlloc(m_storage, new_capacity * sizeof(T), MEM_COMMIT, MEMORY_PROTECTION_LEVEL)) {
			m_capacity = new_capacity;
			return true;
		}

		return false;
	}

	template <typename T>
	bool ObjectPool<T>::extend_capacity(size_type minimum_capacity)
	{
		assert((m_capacity <= std::numeric_limits<size_type>::max() / 2, "Integer overflow"));
		return reserve(std::max(m_capacity * 2, minimum_capacity));
	}

	template <typename T>
	auto ObjectPool<T>::get_free_object_index() const -> index_type
	{
		for (index_type i = 0; i < m_bitmap_size; i++) {
			if (auto mask = m_free_object_bitmaps[i]) {
				unsigned long bit_index;
				_BitScanForward64(&bit_index, mask);
				return i * SIZE_TYPE_BIT_SIZE + bit_index;
			}
		}
		return std::numeric_limits<index_type>::max();
	}


	template <typename T>
	int ObjectPool<T>::transition_to_ready(index_type object_id)
	{
		auto bitmap = &m_free_object_bitmaps[object_id / SIZE_TYPE_BIT_SIZE];
		auto bitmap_index = object_id % SIZE_TYPE_BIT_SIZE;

		*bitmap &= ~(1ULL << bitmap_index);
		return int(object_id);
	}


	template <typename T>
	bool ObjectPool<T>::transition_to_free(int object_id)
	{
		assert(object_id >= 0);
		auto bitmap = &m_free_object_bitmaps[object_id / SIZE_TYPE_BIT_SIZE];
		auto bitmap_index = object_id % SIZE_TYPE_BIT_SIZE;
		bool prev_state = *bitmap & (1ULL << bitmap_index);

		*bitmap |= 1ULL << bitmap_index;
		return prev_state == false;
	}
}