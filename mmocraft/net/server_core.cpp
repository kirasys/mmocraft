#include "pch.h"
#include "server_core.h"

#include "logging/error.h"

namespace net
{
	namespace ServerHandler
	{
		DEFINE_HANDLER(handle_accept)
		{
			static_assert(std::is_same_v<io::IoContext::handler_type, decltype(&handle_accept)>, "Incorrect handler signature");

			if (error_code != ERROR_SUCCESS) {
				logging::cerr() << "handle_accept() failed with " << error_code;
				return;
			}

			auto& server = *static_cast<net::ServerCore*>(event_owner);

			util::defer rearm_server = [&server] {
				server.accept_next_client();
			};

			auto& io_ctx = *io_ctx_ptr;
			auto client_socket = win::UniqueSocket(io_ctx.details.accept.accepted_socket);

			// inherit the properties of the listen socket.
			win::Socket listen_sock = server.get_listen_socket();
			if (SOCKET_ERROR == ::setsockopt(client_socket,
				SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
				reinterpret_cast<char*>(&listen_sock), sizeof(listen_sock))) {
				logging::cerr() << "setsockopt(SO_UPDATE_ACCEPT_CONTEXT) failed";
				return;
			}

			// add a client to the server.
			if (not server.new_connection(std::move(client_socket))) {
				logging::cerr() << "fail to create a server for single client";
				return;
			}
		}
	}
}

namespace net
{
	ServerCore::ServerCore(std::string_view ip, int port,
							unsigned max_client_connections,
							unsigned num_of_event_threads,
							int concurrency_hint)
		: m_server_info{ .ip = ip,
						 .port = port, 
						 .max_client_connections = max_client_connections,
						 .num_of_event_threads = num_of_event_threads}
		, m_listen_sock{ net::SocketType::TCPv4 }
		, m_io_service{ concurrency_hint }
		, m_io_context_pool{ 2 * max_client_connections }
		, m_accept_context{ *IoContextPool::find_object(
								m_io_context_pool.new_object_unsafe(ServerHandler::handle_accept)) }
		, m_connection_server_pool{ max_client_connections }
		, m_interval_task_scheduler{ this }
	{	
		m_io_service.register_event_source(m_listen_sock.get_handle(), /*.event_owner = */ this);

		//
		// Schedule interval tasks.
		// 
		m_interval_task_scheduler.schedule("keep-alive", &ServerCore::check_connection_expiration, util::Second(10));
	}

	bool ServerCore::new_connection(win::UniqueSocket &&client_sock)
	{
		// create a server for single client.
		auto connection_server_id = m_connection_server_pool.new_object(
			std::move(client_sock),
			/* main_server = */ *this,
			m_io_service,
			m_io_context_pool
		);

		auto connection_server = ConnectionServerPool::find_object(connection_server_id);
		if (not connection_server || not connection_server->is_valid())
			return false;

		m_connection_list.push_back(connection_server);
		connection_server_id.release();

		return true;
	}

	void ServerCore::check_connection_expiration()
	{
		for (auto it = m_connection_list.begin(); it != m_connection_list.end();) {
			auto &connection_server = **it;

			if (connection_server.is_safe_delete()) {
				m_connection_server_pool.free_object(&connection_server);
				it = m_connection_list.erase(it);
				continue;
			}

			if (connection_server.is_expired())
				connection_server.set_offline();

			++it;
		}
	}

	void ServerCore::accept_next_client()
	{
		// check there are expired tasks before accepting next client.
		m_interval_task_scheduler.process_tasks();

		try {
			m_listen_sock.accept(m_accept_context);
		}
		catch (...) {
			// TODO: accept scheduling
			logging::cerr() << "fail to request accept";
		}
	}

	void ServerCore::serve_forever()
	{
		m_listen_sock.bind(m_server_info.ip, m_server_info.port);
		m_listen_sock.listen();
		m_listen_sock.accept(m_accept_context);
		std::cout << "Listening to " << m_server_info.ip << ':' << m_server_info.port << "...\n";

		for (unsigned i = 0; i < m_server_info.num_of_event_threads; i++)
			m_io_service.spawn_event_loop_thread().detach();

		m_io_service.run_event_loop_forever();
	}
}