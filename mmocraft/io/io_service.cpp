#include "pch.h"
#include "io_service.h"

#include "logging/error.h"
#include "logging/logger.h"
#include "net/deferred_packet.h"

namespace io
{
    IoCompletionPort::IoCompletionPort(int num_of_concurrent_threads)
        : _handle{ ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, ULONG_PTR(0), num_of_concurrent_threads) }
    { }

    void IoCompletionPort::register_event_source(win::Handle event_source, IoEventHandler* event_handler)
    {
        if (::CreateIoCompletionPort(event_source, _handle, ULONG_PTR(event_handler), DWORD(0)) == NULL)
            throw error::IO_SERVICE_CREATE_COMPLETION_PORT;
    }

    void IoCompletionPort::register_event_source(win::Socket event_source, IoEventHandler* event_handler)
    {
        register_event_source(win::Handle(event_source), event_handler);
    }

    bool IoCompletionPort::push_event(void* event, ULONG_PTR event_handler_inst)
    {
        return ::PostQueuedCompletionStatus(_handle, 
            DWORD(io::CUSTOM_EVENT_SIGNAL),
            event_handler_inst,
            LPOVERLAPPED(event)) != 0;
    }

    void IoCompletionPort::close() noexcept
    {
        _handle.reset();
    }

    void IoCompletionPort::run_event_loop_forever(DWORD get_event_timeout_ms)
    {
        while (true) {
            DWORD transferred_bytes_or_signal = 0;
            ULONG_PTR completion_key = 0;
            LPOVERLAPPED overlapped = nullptr;

            BOOL ok = ::GetQueuedCompletionStatus(
                _handle,
                &transferred_bytes_or_signal,
                &completion_key,
                &overlapped,
                get_event_timeout_ms);

            DWORD error_code = ok ? ERROR_SUCCESS : ::GetLastError();

            // ERROR_ABANDONED_WAIT_0 when completion port handle closed.
            // ref. https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getqueuedcompletionstatus#remarks
            if (error_code == ERROR_ABANDONED_WAIT_0)
                return;

            if (error_code != ERROR_SUCCESS || overlapped == nullptr) {
                LOG(error) << "GetQueuedCompletionStatus() failed with " << error_code;
                continue;
            }

            try {
                if (transferred_bytes_or_signal == CUSTOM_EVENT_SIGNAL) {
                    auto packet_event = reinterpret_cast<net::PacketEvent*>(overlapped);

                    packet_event->invoke_handler(completion_key);
                }
                else {
                    auto io_event = CONTAINING_RECORD(overlapped, io::IoEvent, overlapped);
                    auto event_handler = reinterpret_cast<IoEventHandler*>(completion_key);

                    io_event->invoke_handler(*event_handler, transferred_bytes_or_signal);
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

    std::thread IoCompletionPort::spawn_event_loop_thread()
    {
        return std::thread( [] (IoCompletionPort io_service) {
                io_service.run_event_loop_forever();
            }, *this
        );
    }
}