#pragma once

#include <memory>

#include "io/io_service.h"
#include "net/socket.h"
#include "win/object_pool.h"
#include "win/smart_handle.h"
#include "util/common_util.h"

namespace net
{
	class ServerCore;

	enum ConnectionServerStatus
	{
		OFFLINE,
		ONLINE
	};

	class SingleConnectionServer : util::NonCopyable, util::NonMovable
	{
		using IoContextPool = win::ObjectPool<io::IoContext>;

	public:
		SingleConnectionServer(win::UniqueSocket&&, ServerCore&, io::IoCompletionPort& , IoContextPool&);

		bool is_valid() const
		{
			return m_send_context != nullptr && m_recv_context != nullptr;
		}

		void request_recv_client();
		
		//void send_to_client();

	private:
		net::Socket m_client_socket;

		ServerCore &m_main_server;

		io::IoContext* const m_send_context;
		io::IoContext* const m_recv_context;
		IoContextPool::ScopedID m_send_context_id;
		IoContextPool::ScopedID m_recv_context_id;
	};
}