#pragma once

#include <memory>

#include "io/io_context.h"
#include "net/socket.h"
#include "win/object_pool.h"
#include "win/smart_handle.h"
#include "util/common_util.h"

namespace net
{
	class ServerCore;

	class SingleConnectionServer : util::NonCopyable, util::NonMovable
	{
		using IoContextPool = win::ObjectPool<io::IoContext>;

	public:
		SingleConnectionServer(win::Socket, ServerCore&, IoContextPool::ScopedID&&, IoContextPool::ScopedID&&);

		bool request_recv_client();
		
		//void send_to_client();

	private:
		net::Socket m_client_socket;

		ServerCore &m_main_server;

		io::IoContext &m_send_context;
		io::IoContext &m_recv_context;
		IoContextPool::ScopedID m_send_context_id;
		IoContextPool::ScopedID m_recv_context_id;
	};
}