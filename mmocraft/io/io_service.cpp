#include "pch.h"
#include "io_service.h"

#include "logging/error.h"
#include "io/io_context.h"

namespace io
{
	IoCompletionPort::IoCompletionPort(int num_of_concurrent_threads)
		: m_handle{ ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, ULONG_PTR(0), num_of_concurrent_threads) }
	{ }

	IoCompletionPort::IoCompletionPort(IoCompletionPort&& iocp) noexcept
	{
		m_handle = std::move(iocp.m_handle);
	}

	IoCompletionPort& IoCompletionPort::operator=(IoCompletionPort&& iocp) noexcept
	{
		m_handle = std::move(iocp.m_handle);
		return *this;
	}

	bool IoCompletionPort::register_event_source(win::Handle event_source, void* event_owner)
	{
		if (::CreateIoCompletionPort(event_source, m_handle, ULONG_PTR(event_owner), DWORD(0)) == NULL) {
			logging::cerr() << "Can't associate handle: CreateIoCompletionPort() with " << ::GetLastError();
			return false;
		}
		return true;
	}

	bool IoCompletionPort::register_event_source(win::Socket event_source, void* event_owner)
	{
		return register_event_source(win::Handle(event_source), event_owner);
	}

	void IoCompletionPort::close() noexcept
	{
		if (is_valid()) {
			::CloseHandle(m_handle);
			m_handle.reset();
		}
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
				auto event_owner = reinterpret_cast<void*>(completion_key);
				auto io_ctx = CONTAINING_RECORD(overlapped, io::IoContext, overlapped);
				io_ctx->handler(event_owner, io_ctx, num_of_transferred_bytes, error_code);
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