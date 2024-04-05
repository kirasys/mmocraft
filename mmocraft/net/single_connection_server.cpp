#include "pch.h"
#include "single_connection_server.h"

#include "server_core.h"
#include "logging/error.h"

namespace net
{
	namespace ServerHandler
	{
		DEFINE_HANDLER(handle_send)
		{
			static_assert(std::is_same_v<io::IoContext::handler_type, decltype(&handle_send)>, "Incorrect handler signature");


		}

		DEFINE_HANDLER(handle_recv)
		{
			static_assert(std::is_same_v<io::IoContext::handler_type, decltype(&handle_recv)>, "Incorrect handler signature");

			if (num_of_transferred_bytes == 0) { // client disconnected.
				return;
			}

			auto connection_server = reinterpret_cast<SingleConnectionServer*>(event_owner);
			std::cout << connection_server->get_recv_buffer() << '\n';

			connection_server->request_recv_client();
		}
	}

	SingleConnectionServer::SingleConnectionServer(win::UniqueSocket&& sock,
								ServerCore &main_server,
								io::IoCompletionPort& io_service,
								IoContextPool &io_context_pool)
		: m_client_socket{ std::move(sock) }
		, m_main_server{ main_server }
		, m_send_context_id{ io_context_pool.new_object(ServerHandler::handle_send) }
		, m_recv_context_id{ io_context_pool.new_object(ServerHandler::handle_recv) }
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