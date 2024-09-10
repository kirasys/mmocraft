#pragma once

#include <vector>
#include <thread>
#include <memory>

#include "io/task.h"
#include "io/io_event.h"
#include "logging/error.h"
#include "win/win_type.h"
#include "win/win_base_object.h"
#include "win/smart_handle.h"

namespace io
{
    const int DEFAULT_NUM_OF_CONCURRENT_EVENT_THREADS = 0;
    // 0 means the number of threads concurrently running threads as many processors.
    
    class IoService
    {
    public:
        virtual ~IoService() = default;

        virtual void register_event_source(win::Handle event_source, IoEventHandler* event_handler) = 0;

        virtual void register_event_source(win::Socket event_source, IoEventHandler* event_handler) = 0;

        virtual void run_event_loop_forever(DWORD get_event_timeout_ms) = 0;

        virtual void spawn_event_loop_thread() = 0;
    };

    class IoCompletionPort : public IoService
    {
    public:

        IoCompletionPort(int num_of_concurrent_threads = DEFAULT_NUM_OF_CONCURRENT_EVENT_THREADS);

        ~IoCompletionPort()
        {
            close();
        }
        
        void close() noexcept;

        // copy controllers
        IoCompletionPort(IoCompletionPort& iocp) = default;
        IoCompletionPort& operator=(IoCompletionPort&) = default;

        // move controllers
        IoCompletionPort(IoCompletionPort&& iocp) = default;
        IoCompletionPort& operator=(IoCompletionPort&& iocp) = default;

        void register_event_source(win::Handle event_source, IoEventHandler* event_handler);

        void register_event_source(win::Socket event_source, IoEventHandler* event_handler);

        bool schedule_task(io::Task* task, void* task_handler_inst = nullptr);

        int dequeue_event_results(io::IoEventResult*, std::size_t max_results);

        void run_event_loop_forever(DWORD get_event_timeout_ms = INFINITE);

        void spawn_event_loop_thread();

    private:

        win::UniqueHandle _handle;

        std::vector<std::thread> event_threads;
    };

    class RioCompletionQueue : public util::NonCopyable
    {
    public:
        RioCompletionQueue(std::size_t queue_size, WSAOVERLAPPED*, void* completion_key);

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

    private:
        std::size_t _pool_size = 0;
        std::size_t _buffer_size = 0;
        void* _buffer = nullptr;
        RIO_BUFFERID _buffer_id = RIO_INVALID_BUFFERID;
    };

    class RegisteredIO final : public util::NonCopyable
    {
    public:
        static constexpr std::size_t MAX_IO_EVENT_RESULTS = 128;
        static constexpr std::size_t MAX_RIO_EVENT_RESULTS = 512;

        RegisteredIO(std::size_t max_connections);

        ~RegisteredIO();

        void notify_completion();

        static RIO_BUFFERID create_buffer(void* buffer, std::size_t buffer_size);

        io::IoRecvEvent* create_recv_io_event(unsigned connection_id);

        io::IoSendEvent* create_send_io_event(unsigned connection_id);

        void register_event_source(unsigned connection_id, win::Socket event_source, io::IoEventHandler* event_handler);

        void register_event_source(win::Handle event_source, IoEventHandler* event_handler);

        void register_event_source(win::Socket event_source, IoEventHandler* event_handler);

        int dequeue_event_results(io::IoEventResult*, std::size_t max_results);

        int dequeue_rio_event_result(io::RioEventResult*, std::size_t max_results);

        bool recv(unsigned connection_id, std::byte* buf, std::size_t buf_size, void* context);

        bool send(unsigned connection_id, std::byte* buf, std::size_t buf_size, void* context);

        bool schedule_task(io::Task* task, void* task_handler_inst = nullptr);

        void spawn_event_thread();

        void run_event_loop_forever();

    private:
        const std::size_t _max_connections;

        io::RioCompletionQueue completion_queue;

        std::vector<RIO_RQ> request_queues;

        RioBufferPool recv_buffer_pool;
        RioBufferPool send_buffer_pool;

        std::vector<std::thread> event_threads;

        io::RioEvent _event;
    };
}