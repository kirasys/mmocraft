#include "pch.h"
#include "io_service.h"

#include "logging/error.h"

namespace io
{
	IoCompletionPort::IoCompletionPort(int num_of_concurrent_threads)
		: m_handle{ ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, ULONG_PTR(0), num_of_concurrent_threads) }
	{ }

	void IoCompletionPort::register_event_source(win::Handle event_source, void* event_owner)
	{
		if (::CreateIoCompletionPort(event_source, m_handle, ULONG_PTR(event_owner), DWORD(0)) == NULL)
			throw error::IoException(error::ErrorCode::IO_SERVICE_CREATE_COMPLETION_PORT);
	}

	void IoCompletionPort::register_event_source(win::Socket event_source, void* event_owner)
	{
		register_event_source(win::Handle(event_source), event_owner);
	}

	void IoCompletionPort::close() noexcept
	{
		m_handle.reset();
	}

	void IoCompletionPort::run_event_loop_forever(DWORD get_event_timeout_ms)
	{
		while (true) {
			DWORD num_of_transferred_bytes = 0;
			ULONG_PTR completion_key = 0;
			LPOVERLAPPED overlapped = nullptr;

			BOOL ok = ::GetQueuedCompletionStatus(
				m_handle,
				&num_of_transferred_bytes,
				&completion_key,
				&overlapped,
				get_event_timeout_ms);

			DWORD error_code = ok ? ERROR_SUCCESS : ::GetLastError();

			// ERROR_ABANDONED_WAIT_0 when completion port handle closed.
			// ref. https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getqueuedcompletionstatus#remarks
			if (error_code == ERROR_ABANDONED_WAIT_0)
				return;

			if (error_code != ERROR_SUCCESS) {
				logging::cerr() << "GetQueuedCompletionStatus() failed with " << error_code;
				continue;
			}

			if (overlapped) {
				auto io_event = CONTAINING_RECORD(overlapped, io::IoEvent, overlapped);

				try {
					auto event_handler = reinterpret_cast<IoEventHandler*>(completion_key);
					io_event->invoke_handler(*event_handler, num_of_transferred_bytes);
				}
				catch (const error::Exception& ex) {
					logging::cerr() << "Exception(" << ex.code <<") was caught, but suppressed...";
				}
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