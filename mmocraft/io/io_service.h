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
#include "win/registered_io.h"

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

    class RegisteredIO final : public util::NonCopyable
    {
    public:
        static constexpr std::size_t MAX_IO_EVENT_RESULTS = 128;
        static constexpr std::size_t MAX_RIO_EVENT_RESULTS = 512;

        RegisteredIO(std::size_t max_connections);

        ~RegisteredIO();

        void notify_completion();

        io::IoRecvEvent* create_recv_io_event(unsigned connection_id);

        io::IoSendEvent* create_send_io_event(unsigned connection_id);

        void register_event_source(unsigned connection_id, win::Socket event_source, io::IoEventHandler* event_handler);

        void register_event_source(win::Handle event_source, IoEventHandler* event_handler);

        void register_event_source(win::Socket event_source, IoEventHandler* event_handler);

        int dequeue_event_results(io::IoEventResult*, std::size_t max_results);

        int dequeue_rio_event_result(io::RioEventResult*, std::size_t max_results);

        bool recv(unsigned connection_id, std::byte* buf, std::size_t buf_size, void* context);

        bool send(unsigned connection_id, std::byte* buf, std::size_t buf_size, void* context);

        bool multicast_send(unsigned connection_id, RIO_BUFFERID buffer_id, std::size_t buf_size, void* context);

        bool schedule_task(io::Task* task, void* task_handler_inst = nullptr);

        void spawn_event_thread();

        void run_event_loop_forever();

    private:
        const std::size_t _max_connections;

        win::RioCompletionQueue completion_queue;

        std::vector<RIO_RQ> request_queues;

        win::RioBufferPool recv_buffer_pool;
        win::RioBufferPool send_buffer_pool;

        std::vector<std::thread> event_threads;

        io::RioEvent _event;
    };
}