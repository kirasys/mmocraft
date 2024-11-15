#include "pch.h"
#include "registered_io.h"

#include "net/socket.h"

namespace win
{
    RioCompletionQueue::RioCompletionQueue(std::size_t queue_size, int num_of_concurrent_threads, WSAOVERLAPPED* overlapped, void* completion_key)
        : _iocp_handle{ ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, ULONG_PTR(0), num_of_concurrent_threads) }
        , _cq_handle{ create_complete_queue(queue_size, iocp_handle(), overlapped, completion_key) }
    { }

    void RioCompletionQueue::reset()
    {
        if (is_valid()) {
            net::rio_api().RIOCloseCompletionQueue(_cq_handle);
            _cq_handle = RIO_INVALID_CQ;
        }

        _iocp_handle.reset();
    }

    RIO_CQ RioCompletionQueue::create_complete_queue
    (std::size_t queue_size, win::Handle iocp_handle, WSAOVERLAPPED* overlapped, void* completion_key)
    {
        RIO_NOTIFICATION_COMPLETION cq_type;
        ::ZeroMemory(&cq_type, sizeof(cq_type));
        cq_type.Type = RIO_IOCP_COMPLETION;
        cq_type.Iocp.IocpHandle = iocp_handle;
        cq_type.Iocp.Overlapped = overlapped;
        cq_type.Iocp.CompletionKey = completion_key;

        return net::rio_api().RIOCreateCompletionQueue ?
            net::rio_api().RIOCreateCompletionQueue(DWORD(queue_size), &cq_type) : RIO_INVALID_CQ;
    }

    RioBufferPool::RioBufferPool(std::size_t pool_size, std::size_t buffer_size)
        : _pool_size{ pool_size }
        , _buffer_size{ buffer_size }

        , is_buffer_allocated{ true }
        , _buffer{ ::VirtualAlloc(0, buffer_size * pool_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE) }
        , _buffer_id{ create_buffer(_buffer, buffer_size * pool_size) }
    { }

    RioBufferPool::RioBufferPool(void* buf, std::size_t buf_size)
        : _pool_size{ 1 }
        , _buffer_size{ buf_size }
        , _buffer{ buf }
        , _buffer_id{ create_buffer(_buffer, _buffer_size * _pool_size) }
    { }

    RioBufferPool::~RioBufferPool()
    {
        if (id() != RIO_INVALID_BUFFERID)
            net::rio_api().RIODeregisterBuffer(id());

        if (is_buffer_allocated && _buffer != nullptr)
            ::VirtualFree(_buffer, 0, MEM_RELEASE);
    }

    RIO_BUFFERID RioBufferPool::create_buffer(void* buffer, std::size_t buffer_size)
    {
        return net::rio_api().RIORegisterBuffer ? 
            net::rio_api().RIORegisterBuffer(reinterpret_cast<char*>(buffer), buffer_size) : RIO_INVALID_BUFFERID;
    }
}