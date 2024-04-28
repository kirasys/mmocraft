#include "pch.h"
#include "server_core.h"

#include "logging/error.h"

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

		// No need to release. server core long live until program termination.
		, m_io_event_pool{ *new io::IoEventObjectPool(2 * max_client_connections + 1) }
		, m_accept_event{ *m_io_event_pool.new_accept_event() }

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
			m_io_event_pool
		);

		auto connection_server = ConnectionServerPool::find_object(connection_server_id);
		if (not connection_server || not connection_server->is_valid())
			return false;

		m_connection_list.push_back(connection_server);

		// main server wiil delete connection when expired. (see check_connection_expiration)
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
			m_listen_sock.accept(m_accept_event);
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
		m_listen_sock.accept(m_accept_event);
		std::cout << "Listening to " << m_server_info.ip << ':' << m_server_info.port << "...\n";

		for (unsigned i = 0; i < m_server_info.num_of_event_threads; i++)
			m_io_service.spawn_event_loop_thread().detach();

		m_io_service.run_event_loop_forever();
	}

	/** 
	 * Event handler interface
	 */

	void ServerCore::on_success()
	{
		accept_next_client();
	}

	void ServerCore::on_error()
	{
		
	}

	std::optional<std::size_t> ServerCore::handle_io_event(io::EventType event_type)
	{
		assert(event_type == io::EventType::AcceptEvent);
		return handle_accept_event() ? std::optional<std::size_t>(0) : std::nullopt;
	}

	bool ServerCore::handle_accept_event()
	{
		auto client_socket = win::UniqueSocket(m_accept_event.accepted_socket);

		// inherit the properties of the listen socket.
		win::Socket listen_sock = m_listen_sock.get_handle();
		if (SOCKET_ERROR == ::setsockopt(client_socket,
			SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
			reinterpret_cast<char*>(&listen_sock), sizeof(listen_sock))) {
			logging::cerr() << "setsockopt(SO_UPDATE_ACCEPT_CONTEXT) failed";
			return false;
		}

		// add a client to the server.
		if (not new_connection(std::move(client_socket))) {
			logging::cerr() << "fail to create a server for single client";
			return false;
		}
		
		return true;
	}
}