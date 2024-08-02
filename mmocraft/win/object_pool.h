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
#include <intrin.h>

#include "win/win_type.h"
#include "util/common_util.h"
#include "logging/logger.h"

namespace win
{
    extern std::atomic<std::uint8_t> num_of_free_object_pool;
    extern void* pool_table[];

    enum ObjectPoolErrorCode
    {
        SUCCESS,
        TOO_MANY_POOL_ERROR,
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

    template <typename T, std::size_t OBJECT_SIZE = sizeof(T)>
    class ObjectPool : util::NonCopyable, util::NonMovable
    {
    public:
        using size_type = std::size_t;
        using index_type = std::uint32_t;
        using object_pointer = T*;

        static constexpr int MEMORY_PROTECTION_LEVEL = PAGE_READWRITE;
        static constexpr int SIZE_TYPE_BIT_SIZE = sizeof(size_type) * 8;
        static constexpr size_type DEFAULT_CAPACITY = 512;
        static constexpr index_type INVALID_INDEX = std::numeric_limits<index_type>::max();

        using ObjectID = std::uint64_t;
        static constexpr ObjectID INVALID_OBJECT_ID = 0;

        class Pointer : util::NonCopyable {
        public:
            Pointer() noexcept
                : _id{ INVALID_OBJECT_ID }
            { }

            Pointer(ObjectID id)
                : _id{ id }
            { }

            ~Pointer()
            {
                reset();
            }

            Pointer(Pointer&& other) noexcept
                : _id{ other._id }
            {
                other._id = INVALID_OBJECT_ID;
            }

            Pointer& operator=(Pointer&& other) noexcept
            {
                if (this != &other) {
                    reset();
                    std::swap(_id, other._id);
                }
                return *this;
            }

            object_pointer get()
            {
                return ObjectPool::find_object(_id);
            }

            void release() noexcept
            {
                _id = INVALID_OBJECT_ID;
            }

            bool reset()
            {
                ObjectID id = INVALID_OBJECT_ID;
                std::swap(_id, id);
                return ObjectPool::free_object(id);
            }

            inline operator ObjectID()
            {
                return _id;
            }

            inline operator ObjectID() const
            {
                return _id;
            }

            inline operator bool() const
            {
                return _id != INVALID_OBJECT_ID;
            }

            inline bool to_index() const
            {
                return ObjectPool::get_object_index(_id);
            }

        private:
            ObjectID _id;
        };

        ObjectPool() = delete;

        ObjectPool(size_type max_capacity)
            : _pool_index{ num_of_free_object_pool.fetch_sub(1) }
            , _storage{ nullptr }
            , _capacity{ std::min(DEFAULT_CAPACITY, max_capacity) }
            , _max_capacity{ max_capacity }
            , _bitmap_size{ 1 + max_capacity / SIZE_TYPE_BIT_SIZE }
            , free_storage_bitmaps{ new size_type[_bitmap_size]}
        {
            static_assert(OBJECT_SIZE >= sizeof(T));

            if (not _pool_index)
                throw ObjectPoolErrorCode::TOO_MANY_POOL_ERROR;

            // register this pool to the table;
            pool_table[_pool_index] = this;

            _storage = static_cast<decltype(_storage)>(
                ::VirtualAlloc(NULL, max_capacity * OBJECT_SIZE, MEM_RESERVE, MEMORY_PROTECTION_LEVEL)
                );

            if (_storage == nullptr)
                throw ObjectPoolErrorCode::RESERVE_ERROR;

            if (not ::VirtualAlloc(_storage, _capacity * OBJECT_SIZE, MEM_COMMIT, MEMORY_PROTECTION_LEVEL))
                throw ObjectPoolErrorCode::COMMIT_ERROR;

            std::fill_n(free_storage_bitmaps.get(), _bitmap_size, ~0);
        }

        ~ObjectPool()
        {
            // deregister this pool to the table;
            pool_table[_pool_index] = nullptr;

            if (_storage != nullptr) {
                if (not ::VirtualFree(_storage, 0, MEM_RELEASE))
                    LOG(error) << "VirtualFree() failed with " << ::GetLastError();
            }
        }

        bool reserve(size_type new_capacity)
        {
            if (_capacity >= new_capacity)
                return true;

            if (is_exceed_max_capacity())
                return false;

            new_capacity = std::min(new_capacity, _max_capacity);
            if (::VirtualAlloc(_storage, new_capacity * OBJECT_SIZE, MEM_COMMIT, MEMORY_PROTECTION_LEVEL)) {
                _capacity = new_capacity;
                return true;
            }

            return false;
        }

        // new_object_unsafe returns object's id that must manually free.
        // so it is used only when the user guarantees to free.
        template <typename... Args>
        ObjectID new_object_unsafe(Args&&... args)
        {
            auto object_index = new_object_internal(std::forward<Args>(args)...);
            if (object_index == INVALID_INDEX)
                return INVALID_OBJECT_ID;

            return (ObjectID(_pool_index) << 32) | object_index;
        }

        // new_object_raw returns object as pointer that must manually free.
        // it is used only when the user guarantees to free.
        template <typename... Args>
        object_pointer new_object_raw(Args&&... args)
        {
            auto object_index = new_object_internal(std::forward<Args>(args)...);
            if (object_index == INVALID_INDEX)
                return nullptr;

            return get_object_storage(object_index);
        }

        template <typename... Args>
        Pointer new_object(Args&&... args)
        {
            ObjectID object_id = new_object_unsafe(std::forward<Args>(args)...);
            return object_id != INVALID_OBJECT_ID ? 
                Pointer(object_id) : Pointer();
        }

        bool free_object(object_pointer object_ptr)
        {
            object_ptr->~T();
            return transition_to_free(index_type(object_ptr - get_object_storage(0)));
        }

        static bool free_object(ObjectID object_id)
        {
            if (object_id == INVALID_OBJECT_ID)
                return false;

            if (auto pool = find_pool(object_id)) {
                (pool->get_object_storage(get_object_index(object_id))->~T()); // destruct
                return pool->transition_to_free(get_object_index(object_id));
            }
            return false;
        }

        static object_pointer find_object(ObjectID object_id)
        {
            if (object_id == INVALID_OBJECT_ID)
                return nullptr;

            auto pool = find_pool(object_id);
            return pool ? pool->get_object_storage(get_object_index(object_id)) : nullptr;
        }

        size_type capacity()
        {
            return _capacity;
        }

    private:
        template <typename... Args>
        index_type new_object_internal(Args&&... args)
        {
            auto object_index = get_free_object_index();
            if (object_index >= _capacity && not extend_capacity(size_type(object_index) + 1))
                return INVALID_INDEX;

            auto new_object_storage = get_object_storage(object_index);
            new (new_object_storage) T(std::forward<Args>(args)...);
            // manually construct using placement new.
            // new operator may simply returns its second argument unchanged.
            // ref. https://en.cppreference.com/w/cpp/language/new

            return transition_to_ready(object_index);
        }

        auto get_object_storage(index_type object_index) const
        {
            return reinterpret_cast<object_pointer>(_storage + OBJECT_SIZE * object_index);
        }

        static inline index_type get_object_index(ObjectID id) noexcept
        {
            return id & 0xFFFFFFFF;
        }

        inline bool is_exceed_max_capacity() const
        {
            return _capacity >= _max_capacity;
        }

        static auto find_pool(ObjectID id) noexcept
        {
            return static_cast<ObjectPool*>(pool_table[id >> 32]);
        }

        bool extend_capacity(size_type minimum_capacity)
        {
            return reserve(std::max(_capacity * 2, minimum_capacity));
        }

        index_type get_free_object_index() const
        {
            for (index_type i = 0; i < _bitmap_size; i++) {
                if (auto mask = free_storage_bitmaps[i]) {
                    unsigned long bit_index;
                    _BitScanForward64(&bit_index, mask);
                    return i * SIZE_TYPE_BIT_SIZE + bit_index;
                }
            }
            return std::numeric_limits<index_type>::max();
        }

        index_type transition_to_ready(index_type object_index)
        {
            auto bitmap = &free_storage_bitmaps[object_index / SIZE_TYPE_BIT_SIZE];
            auto bitmap_index = object_index % SIZE_TYPE_BIT_SIZE;

            *bitmap &= ~(1ULL << bitmap_index);
            return object_index;
        }

        bool transition_to_free(index_type object_index)
        {
            assert(object_index >= 0);
            auto bitmap = &free_storage_bitmaps[object_index / SIZE_TYPE_BIT_SIZE];
            auto bitmap_index = object_index % SIZE_TYPE_BIT_SIZE;
            bool prev_state = *bitmap & (1ULL << bitmap_index);

            *bitmap |= 1ULL << bitmap_index;
            return prev_state == false; // check it was in use.
        }
        
        const index_type _pool_index;

        std::byte *_storage;
        size_type _capacity;
        const size_type _max_capacity;
        
        const size_type _bitmap_size;
        std::unique_ptr<size_type[]> free_storage_bitmaps;
    };
}