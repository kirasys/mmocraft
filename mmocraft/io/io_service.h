#pragma once

#include <vector>
#include <thread>
#include <memory>

#include "io/io_context.h"
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
		virtual bool register_event_source(win::Handle event_source, void* event_owner = nullptr) = 0;

		virtual bool register_event_source(win::Socket event_source, void* event_owner = nullptr) = 0;

		virtual void run_event_loop_forever(DWORD get_event_timeout_ms) = 0;

		virtual std::thread spawn_event_loop_thread() = 0;
	};

	class IoCompletionPort : public IoService, private win::WinBaseObject<win::Handle>
	{
	public:
		IoCompletionPort() noexcept
			: m_handle{ }
		{ }

		~IoCompletionPort() = default;

		IoCompletionPort(int num_of_concurrent_threads);
		
		// copy controllers
		IoCompletionPort(IoCompletionPort& iocp) = default;
		IoCompletionPort& operator=(IoCompletionPort&) = default;

		// move controllers
		IoCompletionPort(IoCompletionPort&& iocp) = default;
		IoCompletionPort& operator=(IoCompletionPort&& iocp) = default;

		bool register_event_source(win::Handle event_source, void* event_owner = nullptr);

		bool register_event_source(win::Socket event_source, void* event_owner = nullptr);

		void run_event_loop_forever(DWORD get_event_timeout_ms = INFINITE);

		std::thread spawn_event_loop_thread();

	private:
		win::Handle get_handle() const
		{
			return m_handle;
		}

		bool is_valid() const
		{
			return m_handle;
		}

		void close() noexcept;

		win::SharedHandle m_handle;
	};
}