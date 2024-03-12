#pragma once
#define NOMINMAX
#include <windows.h>

namespace io
{
	class AsyncModel
	{

	};

	class IoCompletionPort : public AsyncModel
	{
	public:
		IoCompletionPort() noexcept
			: m_handle(NULL)
		{ }

		IoCompletionPort(int numOfConcurrentThreads = 0)
			: m_handle(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, numOfConcurrentThreads))
		{ }

		inline bool is_valid() const {
			return m_handle != NULL;
		}

	private:
		HANDLE m_handle;
	};
}