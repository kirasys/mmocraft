#include "pch.h"
#include "server_core.h"

#include <cassert>

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
		, m_io_event_pool{ *new io::IoEventPool(max_client_connections) }
		, m_accept_event{ m_io_event_pool.new_accept_event() }
		, m_connection_server_pool{ max_client_connections }
		, max_online_connection_key{ 0 }
		, online_connection_table{ new ConnectionServer*[max_client_connections + 1]() }
		, m_interval_task_scheduler{ this }
	{	
		m_io_service.register_event_source(m_listen_sock.get_handle(), /*.event_handler = */ this);

		//
		// Schedule interval tasks.
		// 
		m_interval_task_scheduler.schedule("keep-alive", &ServerCore::check_connection_expiration, util::Second(10));
	}

	unsigned ServerCore::issue_online_connection_key()
	{
		shrink_max_online_connection_key();

		for (unsigned i = 0; i < max_online_connection_key; i++) // find free slot.
			if (online_connection_table[i] == nullptr) return i;

		max_online_connection_key += 1;
		assert(max_online_connection_key <= m_server_info.max_client_connections);
		return max_online_connection_key;
	}

	void ServerCore::delete_online_connection_key(unsigned key)
	{
		online_connection_table[key] = nullptr;
		
	}

	void ServerCore::shrink_max_online_connection_key()
	{
		for (unsigned i = max_online_connection_key; i > 0; i--) {
			if (online_connection_table[i]) {
				max_online_connection_key = i;
				return;
			}
		}
		max_online_connection_key = 0;
	}

	void ServerCore::new_connection(win::UniqueSocket &&client_sock)
	{
		// create a server for single client.
		auto connection_server_id = m_connection_server_pool.new_object(
			std::move(client_sock),
			/* main_server = */ *this,
			m_io_service,
			m_io_event_pool
		);

		auto connection_server = ConnectionServerPool::find_object(connection_server_id);
		online_connection_table[connection_server->online_key] = connection_server;
		m_connection_list.emplace_back(std::move(connection_server_id));
	}

	void ServerCore::check_connection_expiration()
	{
		for (auto it = m_connection_list.begin(); it != m_connection_list.end();) {
			auto &connection_server = *ConnectionServerPool::find_object(*it);

			if (connection_server.is_safe_delete()) {
				it = m_connection_list.erase(it);
				continue;
			}

			if (connection_server.is_expired())
				connection_server.set_offline();

			++it;
		}
	}

	void ServerCore::serve_forever()
	{
		m_listen_sock.bind(m_server_info.ip, m_server_info.port);
		m_listen_sock.listen();
		m_listen_sock.accept(*m_accept_event.get());
		std::cout << "Listening to " << m_server_info.ip << ':' << m_server_info.port << "...\n";

		for (unsigned i = 0; i < m_server_info.num_of_event_threads; i++)
			m_io_service.spawn_event_loop_thread().detach();

		m_io_service.run_event_loop_forever();
	}

	/** 
	 * Event handler interface
	 */

	void ServerCore::on_success(io::IoEvent* event)
	{
		// check there are expired tasks before accepting next client.
		m_interval_task_scheduler.process_tasks();

		try {
			m_listen_sock.accept(*static_cast<io::IoAcceptEvent*>(event));
		}
		catch (...) {
			// TODO: accept scheduling
			logging::cerr() << "fail to request accept";
		}
	}

	void ServerCore::on_error(io::IoEvent* event)
	{
		
	}

	std::optional<std::size_t> ServerCore::handle_io_event(io::EventType event_type, io::IoEvent* event)
	{
		assert(event_type == io::EventType::AcceptEvent);
		return handle_accept_event(*static_cast<io::IoAcceptEvent*>(event)) 
					? std::optional<std::size_t>(0) : std::nullopt;
	}

	bool ServerCore::handle_accept_event(io::IoAcceptEvent& event)
	{
		if (m_connection_list.size() > m_server_info.max_client_connections) {
			logging::cerr() << "full connection reached. skip to accept new client";
			return false;
		}

		auto client_socket = win::UniqueSocket(event.accepted_socket);

		// inherit the properties of the listen socket.
		win::Socket listen_sock = m_listen_sock.get_handle();
		if (SOCKET_ERROR == ::setsockopt(client_socket,
			SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
			reinterpret_cast<char*>(&listen_sock), sizeof(listen_sock))) {
			logging::cerr() << "setsockopt(SO_UPDATE_ACCEPT_CONTEXT) failed";
			return false;
		}

		// add a client to the server.
		new_connection(std::move(client_socket));
		return true;
	}
}