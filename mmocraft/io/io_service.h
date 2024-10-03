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
    constexpr int default_num_of_concurrent_event_threads = 0;
    // 0 means the number of threads concurrently running threads as many processors.
    
    class IoServiceModel
    {
    public:
        virtual ~IoServiceModel() = default;

        virtual void register_event_source(win::Handle event_source, IoEventHandler* event_handler) = 0;

        virtual void register_event_source(win::Socket event_source, IoEventHandler* event_handler) = 0;

        virtual void run_event_loop_forever(DWORD get_event_timeout_ms) = 0;

        virtual void spawn_event_thread() = 0;
    };

    class IoCompletionPort : public IoServiceModel
    {
    public:

        IoCompletionPort(int num_of_concurrent_threads = default_num_of_concurrent_event_threads);

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

        void spawn_event_thread();

    private:

        win::UniqueHandle _handle;

        std::vector<std::thread> event_threads;
    };

    class RegisteredIO final : public util::NonCopyable
    {
    public:
        static constexpr std::size_t max_dequeuing_io_event_results = 128;

        static constexpr std::size_t max_dequeuing_rio_event_results = 512;

        RegisteredIO(std::size_t max_connections, int num_of_concurrent_threads = default_num_of_concurrent_event_threads);

        ~RegisteredIO();

        void notify_completion();

        io::IoRecvEvent* create_recv_io_event(unsigned connection_id);

        io::IoSendEvent* create_send_io_event(unsigned connection_id);

        void register_event_source(unsigned connection_id, win::Socket client_sock, io::IoEventHandler* client_connection);

        void register_event_source(win::Handle event_source, IoEventHandler* event_handler);

        void register_event_source(win::Socket client_sock, IoEventHandler* client_connection);

        int dequeue_event_results(io::IoEventResult*, std::size_t max_results);

        int dequeue_rio_event_result(io::RioEventResult*, std::size_t max_results);

        bool recv(unsigned connection_id, std::byte* buf, std::size_t buf_size, void* io_recv_event);

        bool send(unsigned connection_id, std::byte* buf, std::size_t buf_size, void* io_send_event);

        bool multicast_send(unsigned connection_id, RIO_BUFFERID buffer_id, std::size_t buf_size, void* io_send_event);

        bool schedule_task(io::Task* task, void* task_handler = nullptr);

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

    using IoService = RegisteredIO;
}