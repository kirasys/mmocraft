#include "pch.h"
#include "single_connection_server.h"

#include "server_core.h"

namespace net
{
	SingleConnectionServer::SingleConnectionServer(win::Socket sock,
								ServerCore &main_server,
								IoContextPool::ScopedID &&send_context_id,
								IoContextPool::ScopedID &&recv_context_id)
		: m_client_socket{ sock }
		, m_main_server{ main_server }
		, m_send_context_id{ std::move(send_context_id) }
		, m_recv_context_id{ std::move(recv_context_id) }
		, m_send_context{ *IoContextPool::find_object(m_send_context_id) }
		, m_recv_context{ *IoContextPool::find_object(m_recv_context_id) }
	{

	}

	void SingleConnectionServer::request_recv_client()
	{
		m_client_socket.recv(m_recv_context);
	}
}