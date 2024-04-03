#include "pch.h"
#include "single_connection_server.h"

#include "server_core.h"
#include "logging/error.h"

namespace net
{
	SingleConnectionServer::SingleConnectionServer(win::UniqueSocket&& sock,
								ServerCore &main_server,
								io::IoCompletionPort& io_service,
								IoContextPool &io_context_pool)
		: m_client_socket{ std::move(sock) }
		, m_main_server{ main_server }
		, m_send_context_id{ io_context_pool.new_object(ServerCoreHandler::handle_send) }
		, m_recv_context_id{ io_context_pool.new_object(ServerCoreHandler::handle_send) }
		, m_send_context{ IoContextPool::find_object(m_send_context_id) }
		, m_recv_context{ IoContextPool::find_object(m_recv_context_id) }
	{
		// allow to service client socket events.
		io_service.register_event_source(m_client_socket.get_handle(),
									/*.event_owner = */ reinterpret_cast<void*>(this));

		// init first recv.
		this->request_recv_client();
	}

	void SingleConnectionServer::request_recv_client()
	{
		m_client_socket.recv(*m_recv_context);
	}
}