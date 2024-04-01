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

	// ObjectPool
	// - �̸� �Ҵ�� ��ü�� memory-safe �ϰ� ��ȯ���� �� �ִ� Ŭ����.
	// 
	// - ����
	// 1. �޸� �Ҵ�/���� ������� ����
	// 2. locality ����
	//	  - ��� ��ü�� ���ӵ� ���� �ּ� ������ �Ҵ�Ǿ� ��� ����� map �ڷᱸ������ fragmentation�� ����. (WSS ����)
	//    - (?) ������ ��ȸ�� cache hit ���� ����
	// 3. �޸� ȿ����
	//	  - VirtualAlloc���� �ּ� ������ �����صα� ������ ���� �޸� ������ ���� �ּ�ȭ�Ѵ�.
	// 4. (TODO) �޸� Ȯ�� �� �߻��ϴ� ������� ���� �ʿ�
	//    - VirtualAlloc���� Ŀ�� vs std::dequeó�� ûũ ���� ���� (����� VirtualAlloc�� ����)
	// 
	// - ������
	// 1. memory-safey
	//	  - ��ü(object)�� unique_ptr ������� �����ϴ� SafePointer Ŭ������ �����Ͽ� �޸� ������ �����Ѵ�.
	//		- (Decision) �ణ�� �޸� ������尡 ������ �޸� �������� �����Ѵ�.
	//	  - ObjectPool�� ��ü���� ���� �������� �ʵȴ�.
	//		- (Decision) ��κ��� ObjectPool�� ���α׷� ������� �����ϹǷ� ����ũ�� �������� �����Ѵ�.
	// 2. thread-safe
	//    - �⺻������ Ŭ������ thread-safe�ϰ� ������� ����.
	//		- �������� ObjectPool�� ���� ������� ������ ���� ���� ������ ���� �ʴ´�. 
	//	  - ��ü�� �Ҵ��� �� �����忡�� ���������, ��ȯ�� �ٸ� ������ �帧���� ����� �� �־� ��ġ�� �ʿ��ϴ�.
	//		- bitmap search�� �ذ�
	//		  - Ž���� O(n) �ð��� ������, n�� ���� 10000 ������ ������ �ٸ� ���(lock, lock-free ��)�� ���� ���� �Ŷ� ����
	//        - CPU ĳ���� �������� RAM �𵨿��� ������ �ð����� ���� ������ ������ �Ŷ� ����
	//		  - (TODO) ���� ���� �ʿ�
	//    - (TODO) ������ thrad-safe ó���� �ʿ信 ���� �߰� ����.

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
		static constexpr index_type INVALID_INDEX = std::numeric_limits<index_type>::max();

		using ObjectID = std::uint64_t;
		static constexpr ObjectID INVALID_OBJECT_ID = 0;

		class ScopedObjectID : util::NonCopyable {
		public:
			ScopedObjectID() noexcept
				: m_id{ INVALID_OBJECT_ID }
			{ }

			ScopedObjectID(ObjectID id)
				: m_id{ id }
			{ }

			~ScopedObjectID()
			{
				clear();
			}

			ScopedObjectID(ScopedObjectID&& other) noexcept
			{
				m_id = other.m_id;
				other.m_id = INVALID_OBJECT_ID;
			}

			ScopedObjectID& operator=(ScopedObjectID&& other) noexcept
			{
				if (this != &other) {
					clear();
					m_id = other.m_id;
					other.m_id = INVALID_OBJECT_ID;
				}
			}

			void clear()
			{
				ObjectPool::free_object(m_id);
			}

			inline operator ObjectID()
			{
				return m_id;
			}

			inline operator ObjectID() const
			{
				return m_id;
			}

			inline bool is_valid() const
			{
				return m_id != INVALID_OBJECT_ID;
			}

			inline bool get_id() const
			{
				return m_id;
			}

		private:
			ObjectID m_id;
		};

		ObjectPool() = delete;

		ObjectPool(size_type max_capacity);

		~ObjectPool();

		bool reserve(size_type new_capacity);

		// new_object returns object's id that must manually free.
		// so it is used only when the user guarantees to free.

		template <typename... Args>
		ObjectID new_object_unsafe(Args&&... args)
		{
			auto storage_index = new_object_internal(std::forward<Args>(args)...);
			if (storage_index == INVALID_INDEX)
				return INVALID_OBJECT_ID;

			return (ObjectID(m_pool_index) << 32) | storage_index;
		}


		template <typename... Args>
		ScopedObjectID new_object(Args&&... args)
		{
			ObjectID object_id = new_object_unsafe(std::forward<Args>(args)...);
			return object_id != INVALID_OBJECT_ID ? 
				ScopedObjectID(object_id) : ScopedObjectID();
		}

		static bool free_object(ObjectID object_id)
		{
			if (object_id == INVALID_OBJECT_ID)
				return false;

			if (auto pool = static_cast<ObjectPool*>(pool_table[pool_index(object_id)])) {
				(pool->get_storage(storage_index(object_id))->~T()); // destruct
				return pool->transition_to_free(storage_index(object_id));
			}
			return false;
		}

		static object_pointer find_object(ObjectID object_id)
		{
			if (object_id == INVALID_OBJECT_ID)
				return nullptr;

			auto pool = static_cast<ObjectPool*>(pool_table[pool_index(object_id)]);
			return pool ? pool->get_storage(storage_index(object_id)) : nullptr;
		}

		size_type capacity()
		{
			return m_capacity;
		}

	private:
		template <typename... Args>
		index_type new_object_internal(Args&&... args)
		{
			auto storage_index = get_free_storage_index();
			if (storage_index >= m_capacity && not extend_capacity(size_type(storage_index) + 1))
				return INVALID_INDEX;

			auto new_object_storage = &m_storage[storage_index];
			new (new_object_storage) T(std::forward<Args>(args)...);
			// manually construct using placement new.
			// new operator may simply returns its second argument unchanged.
			// ref. https://en.cppreference.com/w/cpp/language/new

			return transition_to_ready(storage_index);
		}

		object_pointer get_storage(index_type storage_index) const
		{
			return &m_storage[storage_index];
		}

		inline bool is_exceed_max_capacity() const
		{
			return m_capacity >= m_max_capacity;
		}

		static inline index_type storage_index(ObjectID id) noexcept
		{
			return id & 0xFFFFFFFF;
		}

		static inline index_type pool_index(ObjectID id) noexcept
		{
			return id >> 32;
		}

		bool extend_capacity(size_type minimum_capacity);

		index_type get_free_storage_index() const;

		index_type transition_to_ready(index_type);

		bool transition_to_free(index_type);
		
		const index_type m_pool_index;

		object_pointer m_storage;
		size_type m_capacity;
		const size_type m_max_capacity;
		
		const size_type m_bitmap_size;
		std::unique_ptr<size_type[]> m_free_storage_bitmaps;
	};
}

/// Definitions

namespace win
{
	template <typename T>
	ObjectPool<T>::ObjectPool(size_type max_capacity)
		: m_pool_index{ num_of_free_object_pool.fetch_sub(1) }
		, m_storage{ nullptr }
		, m_capacity{ std::min(DEFAULT_CAPACITY, max_capacity) }
		, m_max_capacity{ max_capacity }
		, m_bitmap_size{ 1 + max_capacity / SIZE_TYPE_BIT_SIZE }
		, m_free_storage_bitmaps{ new size_type[m_bitmap_size]}
	{
		if (not m_pool_index)
			return;

		// register this pool to the table;
		pool_table[m_pool_index] = this;

		m_storage = static_cast<object_pointer>(
			::VirtualAlloc(NULL, max_capacity * sizeof(T), MEM_RESERVE, MEMORY_PROTECTION_LEVEL)
			);

		if (m_storage == nullptr)
			throw ObjectPoolErrorCode::RESERVE_ERROR;

		if (not ::VirtualAlloc(m_storage, m_capacity * sizeof(T), MEM_COMMIT, MEMORY_PROTECTION_LEVEL))
			throw ObjectPoolErrorCode::COMMIT_ERROR;

		// set bitmaps all object are free.
		for (index_type i = 0; i < m_bitmap_size; i++)
			m_free_storage_bitmaps[i] = ~size_type(0);
	}

	template <typename T>
	ObjectPool<T>::~ObjectPool()
	{
		// deregister this pool to the table;
		pool_table[m_pool_index] = nullptr;

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
	auto ObjectPool<T>::get_free_storage_index() const -> index_type
	{
		for (index_type i = 0; i < m_bitmap_size; i++) {
			if (auto mask = m_free_storage_bitmaps[i]) {
				unsigned long bit_index;
				_BitScanForward64(&bit_index, mask);
				return i * SIZE_TYPE_BIT_SIZE + bit_index;
			}
		}
		return std::numeric_limits<index_type>::max();
	}

	template <typename T>
	auto ObjectPool<T>::transition_to_ready(index_type storage_index) -> index_type
	{
		auto bitmap = &m_free_storage_bitmaps[storage_index / SIZE_TYPE_BIT_SIZE];
		auto bitmap_index = storage_index % SIZE_TYPE_BIT_SIZE;

		*bitmap &= ~(1ULL << bitmap_index);
		return storage_index;
	}


	template <typename T>
	bool ObjectPool<T>::transition_to_free(index_type storage_index)
	{
		assert(storage_index >= 0);
		auto bitmap = &m_free_storage_bitmaps[storage_index / SIZE_TYPE_BIT_SIZE];
		auto bitmap_index = storage_index % SIZE_TYPE_BIT_SIZE;
		bool prev_state = *bitmap & (1ULL << bitmap_index);

		*bitmap |= 1ULL << bitmap_index;
		return prev_state == false;
	}
}