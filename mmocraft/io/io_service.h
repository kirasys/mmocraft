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

        virtual std::thread spawn_event_loop_thread() = 0;
    };

    class IoCompletionPort : public IoService, private win::WinBaseObject<win::Handle>
    {
    public:

        IoCompletionPort(int num_of_concurrent_threads = DEFAULT_NUM_OF_CONCURRENT_EVENT_THREADS);

        ~IoCompletionPort() = default;
        
        // copy controllers
        IoCompletionPort(IoCompletionPort& iocp) = default;
        IoCompletionPort& operator=(IoCompletionPort&) = default;

        // move controllers
        IoCompletionPort(IoCompletionPort&& iocp) = default;
        IoCompletionPort& operator=(IoCompletionPort&& iocp) = default;

        void register_event_source(win::Handle event_source, IoEventHandler* event_handler);

        void register_event_source(win::Socket event_source, IoEventHandler* event_handler);

        bool schedule_task(io::Task* task, void* task_handler_inst = nullptr);

        void run_event_loop_forever(DWORD get_event_timeout_ms = INFINITE);

        std::thread spawn_event_loop_thread();

    private:
        win::Handle get_handle() const
        {
            return _handle;
        }

        bool is_valid() const
        {
            return _handle;
        }

        void close() noexcept;

        win::SharedHandle _handle;
    };
}