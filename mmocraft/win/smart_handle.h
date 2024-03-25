#pragma once

#include <memory>
#include <utility>
#include <type_traits>

#include "win_type.h"
#include "util/common_util.h"

namespace {
	void invalid_handle_deleter(win::Handle h) { }
	
	void invalid_socket_deleter(win::Socket* h) { }

	void socket_deleter(win::Socket* h) {
		::closesocket(win::Socket(h));
	}
}

namespace win
{
	class UniqueSocket : util::NonCopyable
	{
	public:
		UniqueSocket() noexcept
			: m_handle{nullptr, invalid_socket_deleter}
		{ }

		UniqueSocket(win::Socket handle) noexcept
			: m_handle(reinterpret_cast<win::Socket*>(handle), socket_deleter)
		{ }

		operator win::Socket()
		{
			return win::Socket(m_handle.get());
		}

		operator win::Socket() const
		{
			return win::Socket(m_handle.get());
		}

		win::Socket get() const
		{
			return win::Socket(m_handle.get());
		}
		
		void reset()
		{
			m_handle.reset();
		}

		// move controllers
		UniqueSocket(UniqueSocket&& handle) = default;
		UniqueSocket& operator=(UniqueSocket&& handle) = default;

	private:
		std::unique_ptr<std::remove_pointer_t<win::Socket>, decltype(socket_deleter)*> m_handle;
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