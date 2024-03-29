#pragma once

#include <memory>

#include "io/io_context.h"
#include "win/object_pool.h"
#include "win/smart_handle.h"
#include "util/common_util.h"

namespace net
{
	class ClientSession : util::NonCopyable
	{
	using IoContextKey = win::ObjectPool<io::IoContext>::ObjectKey;
	public:
		ClientSession() noexcept
			: m_socket{ }, m_send_io_ctx{ }, m_recv_io_ctx{ }
		{ }

		ClientSession(win::Socket socket, IoContextKey send_io_ctx, IoContextKey recv_io_ctx)
			: m_socket{ socket }
			, m_send_io_ctx{ std::move(send_io_ctx) }
			, m_recv_io_ctx{ std::move(recv_io_ctx) }
		{ }

		// move controllers
		ClientSession(ClientSession&&) = default;
		ClientSession& operator=(ClientSession&&) = default;

	private:
		win::UniqueSocket m_socket;
		
		IoContextKey m_send_io_ctx;
		IoContextKey m_recv_io_ctx;
	};
}