#pragma once

#include <cassert>

#include "win/win_type.h"
#include "win/smart_handle.h"
#include "util/common_util.h"

namespace win
{
    class RioCompletionQueue : public util::NonCopyable
    {
    public:
        RioCompletionQueue(std::size_t queue_size, int num_of_concurrent_threads, WSAOVERLAPPED*, void* completion_key);

        ~RioCompletionQueue()
        {
            reset();
        }

        win::Handle iocp_handle() const
        {
            return _iocp_handle.get();
        }

        RIO_CQ rio_handle() const
        {
            return _cq_handle;
        }

        bool is_valid() const
        {
            return _iocp_handle.is_valid() && _cq_handle != RIO_INVALID_CQ;
        }

        void reset();

        static RIO_CQ create_complete_queue(std::size_t queue_size, win::Handle, WSAOVERLAPPED*, void* completion_key);

    private:
        win::UniqueHandle _iocp_handle;
        RIO_CQ _cq_handle = RIO_INVALID_CQ;
    };

    class RioBufferPool : public util::NonCopyable
    {
    public:
        RioBufferPool(std::size_t pool_size, std::size_t buffer_size);

        RioBufferPool(void* buf, std::size_t buf_size);

        ~RioBufferPool();

        std::size_t offset(unsigned index = 0) const
        {
            assert(index < _pool_size);
            return _buffer_size * index;
        }

        std::byte* buffer(unsigned index = 0) const
        {
            assert(index < _pool_size);
            return reinterpret_cast<std::byte*>(_buffer) + offset(index);
        }

        std::size_t buffer_size() const
        {
            return _buffer_size;
        }

        RIO_BUFFERID id() const
        {
            return _buffer_id;
        }

        bool is_valid() const
        {
            return _buffer != nullptr && id() != RIO_INVALID_BUFFERID;
        }

        static RIO_BUFFERID create_buffer(void* buffer, std::size_t buffer_size);

    private:
        std::size_t _pool_size = 0;
        std::size_t _buffer_size = 0;

        bool is_buffer_allocated = false;
        void* _buffer = nullptr;
        RIO_BUFFERID _buffer_id = RIO_INVALID_BUFFERID;
    };
}