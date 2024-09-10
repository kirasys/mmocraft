#include "pch.h"
#include "io_service.h"

#include "net/packet.h"
#include "net/socket.h"
#include "logging/error.h"
#include "logging/logger.h"

namespace
{

}

namespace io
{
    IoCompletionPort::IoCompletionPort(int num_of_concurrent_threads)
        : _handle{ ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, ULONG_PTR(0), num_of_concurrent_threads) }
    { }

    void IoCompletionPort::register_event_source(win::Handle event_source, IoEventHandler* event_handler)
    {
        if (::CreateIoCompletionPort(event_source, _handle.get(), ULONG_PTR(event_handler), DWORD(0)) == NULL)
            CONSOLE_LOG(error) << "Unable to attach io completion port.";
    }

    void IoCompletionPort::register_event_source(win::Socket event_source, IoEventHandler* event_handler)
    {
        register_event_source(win::Handle(event_source), event_handler);
    }

    bool IoCompletionPort::schedule_task(io::Task* task, void* task_handler_inst)
    {
        task->before_scheduling();

        return ::PostQueuedCompletionStatus(_handle.get(), 0, ULONG_PTR(task_handler_inst), &task->overlapped) != 0;
    }

    int IoCompletionPort::dequeue_event_results(io::IoEventResult* event_results, std::size_t max_results)
    {
        ULONG num_of_results = 0;

        BOOL ok = ::GetQueuedCompletionStatusEx(
            _handle.get(),
            event_results,
            ULONG(max_results),
            &num_of_results,
            INFINITE,
            FALSE
        );

        return ok || ::GetLastError() != ERROR_ABANDONED_WAIT_0 ? int(num_of_results) : -1;
    }

    void IoCompletionPort::close() noexcept
    {
        _handle.reset();

        for (auto& thread : event_threads)
            thread.join();

        event_threads.clear();
    }

    void IoCompletionPort::run_event_loop_forever(DWORD get_event_timeout_ms)
    {
        io::IoEventResult event_results[64];

        while (true) {
            auto num_results = dequeue_event_results(event_results, std::size(event_results));
            if (num_results < 0)
                return;

            for (int i = 0; i < num_results; i++) {
                if (event_results[i].lpOverlapped == nullptr) {
                    LOG(error) << "GetQueuedCompletionStatus() failed";
                    continue;
                }

                auto transferred_bytes_or_signal = event_results[i].dwNumberOfBytesTransferred;

                try {
                    if (transferred_bytes_or_signal == IO_TASK_SIGNAL) {
                        auto task = reinterpret_cast<io::Task*>(event_results[i].lpOverlapped);

                        //task->invoke_handler(event_results[i].lpCompletionKey);
                    }
                    else {
                        auto io_event = CONTAINING_RECORD(event_results[i].lpOverlapped, io::IoEvent, overlapped);
                        auto event_handler = reinterpret_cast<IoEventHandler*>(event_results[i].lpCompletionKey);

                        //io_event->invoke_handler(*event_handler, transferred_bytes_or_signal, ERROR_SUCCESS);
                    }
                }
                catch (error::ErrorCode error_code) {
                    LOG(error) << "Exception was caught with " << error_code << ", buf supressed..";
                }
                catch (...) {
                    LOG(error) << "Unexcepted exception was caught, but suppressed...";
                }
            }
        }
    }

    void IoCompletionPort::spawn_event_loop_thread()
    {
        event_threads.emplace_back(std::thread([](IoCompletionPort* io_service) {
            io_service->run_event_loop_forever();
            }, this
        ));
    }

    RioCompletionQueue::RioCompletionQueue(std::size_t queue_size, WSAOVERLAPPED* overlapped, void* completion_key)
        : _iocp_handle{ ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, ULONG_PTR(0), 0) }
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

        return net::rio_api().RIOCreateCompletionQueue(DWORD(queue_size), &cq_type);
    }

    RioBufferPool::RioBufferPool(std::size_t pool_size, std::size_t buffer_size)
        : _pool_size{ pool_size }
        , _buffer_size{ buffer_size }
        , _buffer{ ::VirtualAlloc(0, buffer_size * pool_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE) }
        , _buffer_id{ RegisteredIO::create_buffer(_buffer, buffer_size * pool_size) }
    { }

    RioBufferPool::~RioBufferPool()
    {
        if (id() != RIO_INVALID_BUFFERID)
            net::rio_api().RIODeregisterBuffer(id());

        if (_buffer != nullptr)
            ::VirtualFree(_buffer, 0, MEM_RELEASE);
    }

    RegisteredIO::RegisteredIO(std::size_t max_connections)
        : _max_connections{ max_connections }
        , completion_queue{ 2 * max_connections, &_event.overlapped, this }
        , request_queues{ max_connections, RIO_INVALID_RQ }
        , recv_buffer_pool{ max_connections, io::RECV_BUFFER_SIZE }
        , send_buffer_pool{ max_connections, io::SEND_BUFFER_SIZE }
    {
        CONSOLE_LOG_IF(fatal, not completion_queue.is_valid()) << "Fail to create completion queue.";
        CONSOLE_LOG_IF(fatal, not recv_buffer_pool.is_valid()) << "Fail to allocate recv buffer.";
        CONSOLE_LOG_IF(fatal, not send_buffer_pool.is_valid()) << "Fail to allocate send buffer.";

        notify_completion();
    }

    RegisteredIO::~RegisteredIO()
    {
        completion_queue.reset();

        for (auto& thread : event_threads)
            thread.join();
        
        event_threads.clear();
    }

    void RegisteredIO::notify_completion()
    {
        auto notified_result = net::rio_api().RIONotify(completion_queue.rio_handle());
        if (notified_result != ERROR_SUCCESS)
            CONSOLE_LOG(error) << "RIONotify failed with:" << ::GetLastError();
    }

    RIO_BUFFERID RegisteredIO::create_buffer(void* buffer, std::size_t buffer_size)
    {
        return net::rio_api().RIORegisterBuffer(reinterpret_cast<char*>(buffer), buffer_size);
    }

    io::IoRecvEvent* RegisteredIO::create_recv_io_event(unsigned connection_id)
    {
        auto io_event_data = new io::IoRecvEventRawData(
            recv_buffer_pool.buffer(connection_id), recv_buffer_pool.buffer_size()
        );

        return new io::IoRecvEvent(io_event_data);
    }

    io::IoSendEvent* RegisteredIO::create_send_io_event(unsigned connection_id)
    {
        auto io_event_data = new io::IoSendEventLockFreeRawData(
            send_buffer_pool.buffer(connection_id), send_buffer_pool.buffer_size()
        );

        return new io::IoSendEvent(io_event_data);
    }

    void RegisteredIO::register_event_source(unsigned connection_id, win::Socket event_source, IoEventHandler* event_handler)
    {
        request_queues[connection_id] = net::rio_api().RIOCreateRequestQueue(
            event_source,
            /* MaxOutstandingReceive =*/ 1,
            /* MaxReceiveDataBuffers =*/ 1,
            /* MaxOutstandingSend =*/ 1,
            /* MaxSendDataBuffers =*/ 1,
            completion_queue.rio_handle(),
            completion_queue.rio_handle(),
            event_handler
        );

        CONSOLE_LOG_IF(error, request_queues[connection_id] == RIO_INVALID_RQ)
            << "Fail to create request queue with " << ::WSAGetLastError();
    }

    void RegisteredIO::register_event_source(win::Handle event_source, IoEventHandler* event_handler)
    {
        if (::CreateIoCompletionPort(event_source, completion_queue.iocp_handle(), ULONG_PTR(event_handler), DWORD(0)) == NULL)
            CONSOLE_LOG(error) << "Unable to attach io completion port.";
    }

    void RegisteredIO::register_event_source(win::Socket event_source, IoEventHandler* event_handler)
    {
        register_event_source(win::Handle(event_source), event_handler);
    }

    int RegisteredIO::dequeue_event_results(io::IoEventResult* event_results, std::size_t max_results)
    {
        ULONG num_results = 0;

        BOOL ok = ::GetQueuedCompletionStatusEx(
            completion_queue.iocp_handle(),
            event_results,
            ULONG(max_results),
            &num_results,
            INFINITE,
            FALSE
        );

        return ok || ::GetLastError() != ERROR_ABANDONED_WAIT_0 ? int(num_results) : -1;
    }

    int RegisteredIO::dequeue_rio_event_result(io::RioEventResult* event_results, std::size_t max_results)
    {
        auto num_results = net::rio_api().RIODequeueCompletion(
            completion_queue.rio_handle(), 
            event_results, 
            max_results);
        
        return num_results != RIO_CORRUPT_CQ ? int(num_results) : -1;
    }

    bool RegisteredIO::recv(unsigned connection_id, std::byte* buf, std::size_t buf_size, void* context)
    {
        RIO_BUF rbuf{
            .BufferId = recv_buffer_pool.id(),
            .Offset = ULONG(buf - recv_buffer_pool.buffer()),
            .Length = ULONG(buf_size)
        };

        DWORD flags = 0;

        if (net::rio_api().RIOReceive(request_queues[connection_id], &rbuf, 1, flags, context) != TRUE) {
            CONSOLE_LOG(error) << "RIOReceive failed with " << ::WSAGetLastError();
            return false;
        }

        return true;
    }

    bool RegisteredIO::send(unsigned connection_id, std::byte* buf, std::size_t buf_size, void* context)
    {
        RIO_BUF rbuf{
            .BufferId = send_buffer_pool.id(),
            .Offset = ULONG(buf - send_buffer_pool.buffer()),
            .Length = ULONG(buf_size)
        };

        DWORD flags = 0;

        if (net::rio_api().RIOSend(request_queues[connection_id], &rbuf, 1, flags, context) != TRUE) {
            CONSOLE_LOG(error) << "RIOSend failed with " << ::WSAGetLastError();
            return false;
        }

        return true;
    }

    bool RegisteredIO::schedule_task(io::Task* task, void* task_handler_inst)
    {
        task->before_scheduling();

        return ::PostQueuedCompletionStatus(
            completion_queue.iocp_handle(),
            0, 
            ULONG_PTR(task_handler_inst), 
            &task->overlapped) != 0;
    }

    void RegisteredIO::run_event_loop_forever()
    {
        io::IoEventResult event_results[MAX_IO_EVENT_RESULTS];

        auto& rio_api = net::rio_api();

        while (true) {
            auto num_results = dequeue_event_results(event_results, std::size(event_results));
            if (num_results < 0) // EOF
                return;

            for (int i = 0; i < num_results; i++) {
                if (event_results[i].lpOverlapped == nullptr) {
                    CONSOLE_LOG(error) << "GetQueuedCompletionStatusEx() failed";
                    continue;
                }

                auto transferred_bytes_or_signal = event_results[i].dwNumberOfBytesTransferred;
                auto completion_key = (void*)event_results[i].lpCompletionKey;
                auto event = CONTAINING_RECORD(event_results[i].lpOverlapped, io::Event, overlapped);

                try {
                    event->on_event_complete(completion_key, transferred_bytes_or_signal);
                    /*
                    if (transferred_bytes_or_signal == IO_TASK_SIGNAL) {
                        auto task = reinterpret_cast<io::Task*>(io_event_results[i].lpOverlapped);
                        task->invoke_handler(io_event_results[i].lpCompletionKey);
                    }
                    else {
                        auto io_event = CONTAINING_RECORD(io_event_results[i].lpOverlapped, io::IoEvent, overlapped);
                        auto event_handler = reinterpret_cast<IoEventHandler*>(io_event_results[i].lpCompletionKey);

                        io_event->invoke_handler(*event_handler, transferred_bytes_or_signal, ERROR_SUCCESS);
                    }
                    */
                }
                catch (error::ErrorCode error_code) {
                    LOG(error) << "Exception was caught with " << error_code << ", buf supressed..";
                }
                catch (...) {
                    LOG(error) << "Unexcepted exception was caught, but suppressed...";
                }
            }
        }
    }

    void RegisteredIO::spawn_event_thread()
    {
        event_threads.emplace_back(std::thread([](RegisteredIO* io_service) {
            io_service->run_event_loop_forever();
            }, this)
        );
    }
}