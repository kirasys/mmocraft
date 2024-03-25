#pragma once

#include <memory>
#include <utility>
#include <type_traits>

#include "win_type.h"

namespace {
	void invalid_handle_deleter(win::Handle h) { }
}

namespace win
{
	class SharedHandle
	{
	public:
		SharedHandle()
			: m_handle{ INVALID_HANDLE_VALUE, invalid_handle_deleter }
		{ }

		SharedHandle(win::Handle handle)
			: m_handle{ handle, ::CloseHandle }
		{ }

		// copy controllers
		SharedHandle(SharedHandle&) = default;
		SharedHandle& operator=(SharedHandle&) = default;

		// move controllers
		SharedHandle(SharedHandle&& handle) = default;
		SharedHandle& operator=(SharedHandle&& handle) = default;

		operator win::Handle()
		{
			return m_handle.get();
		}

		operator win::Handle() const
		{
			return m_handle.get();
		}

		win::Handle get() const
		{
			return m_handle.get();
		}

		// reset operator
		void reset()
		{
			m_handle.reset(INVALID_HANDLE_VALUE, invalid_handle_deleter);
		}

	private:
		std::shared_ptr<std::remove_pointer_t<win::Handle>> m_handle;
	};
}