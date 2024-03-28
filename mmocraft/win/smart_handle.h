#pragma once

#include <memory>
#include <utility>
#include <type_traits>

#include "win_type.h"
#include "util/common_util.h"

namespace {
	void invalid_handle_deleter(win::Handle h) { }
}

namespace win
{
	class UniqueSocket : util::NonCopyable
	{
	public:
		UniqueSocket() noexcept
			: m_handle{INVALID_SOCKET}
		{ }

		UniqueSocket(win::Socket handle) noexcept
			: m_handle(handle)
		{ }

		~UniqueSocket()
		{ 
			clear();
		}

		UniqueSocket(UniqueSocket&& other) noexcept
		{
			m_handle = other.m_handle;
			other.m_handle = INVALID_SOCKET;
		}

		UniqueSocket& operator=(UniqueSocket&& other) noexcept
		{
			if (m_handle != other.m_handle) {
				clear();
				m_handle = other.m_handle;
				other.m_handle = INVALID_SOCKET;
			}
			return *this;
		}

		operator win::Socket()
		{
			return m_handle;
		}

		operator win::Socket() const
		{
			return m_handle;
		}

		win::Socket get()
		{
			return m_handle;
		}

		win::Socket get() const
		{
			return m_handle;
		}
		
		void reset(win::Socket handle = INVALID_SOCKET)
		{
			clear();
			m_handle = handle;
		}

		bool is_valid() const
		{
			return m_handle != INVALID_SOCKET;
		}

	private:
		void clear()
		{
			if (is_valid())
				::closesocket(m_handle);
		}

		win::Socket m_handle;
	};

	using UniqueHandle = std::unique_ptr<std::remove_pointer_t<win::Handle>, decltype(::CloseHandle)*>;
	inline auto make_unique_handle(win::Handle handle)
	{
		return UniqueHandle(handle, ::CloseHandle);
	}

	class SharedHandle
	{
	public:
		SharedHandle()
			: m_handle{ nullptr, invalid_handle_deleter }
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
			m_handle.reset();
		}

	private:
		std::shared_ptr<std::remove_pointer_t<win::Handle>> m_handle;
	};
}