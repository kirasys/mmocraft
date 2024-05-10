#include "pch.h"
#include "server_core.h"

#include <cassert>

#include "logging/error.h"

namespace net
{
	ServerCore::ServerCore(ApplicationServer& a_app_server, std::string_view ip, int port,
							unsigned max_client_connections,
							unsigned num_of_event_threads,
							int concurrency_hint)
		: app_server{ a_app_server }
		, server_info{ .ip = ip,
						 .port = port, 
						 .max_client_connections = max_client_connections,
						 .num_of_event_threads = num_of_event_threads}
		, _listen_sock{ net::SocketType::TCPv4 }
		, io_service{ concurrency_hint }
		, io_event_pool{ max_client_connections }
		, io_accept_event{ io_event_pool.new_accept_event() }
		, connection_server_pool{ max_client_connections }
		, interval_task_scheduler{ this }
	{	
		io_service.register_event_source(_listen_sock.get_handle(), /*.event_handler = */ this);

		ConnectionDescriptorTable::initialize(max_client_connections);

		//
		// Schedule interval tasks.
		// 
		interval_task_scheduler.schedule("keep-alive", &ServerCore::check_connection_expiration, util::Second(10));

		_state = ServerCore::State::Initialized;
	}

	void ServerCore::new_connection(win::UniqueSocket &&client_sock)
	{
		// create a server for single client.
		auto connection_server_id = connection_server_pool.new_object(
			app_server,
			std::move(client_sock),
			io_service,
			io_event_pool
		);

		auto connection_server = ConnectionServerPool::find_object(connection_server_id);
		connection_server_ids.emplace_back(std::move(connection_server_id));
	}

	void ServerCore::check_connection_expiration()
	{
		for (auto it = connection_server_ids.begin(); it != connection_server_ids.end();) {
			auto &connection_server = *ConnectionServerPool::find_object(*it);

			if (connection_server.is_safe_delete()) {
				it = connection_server_ids.erase(it);
				continue;
			}

			if (connection_server.is_expired())
				connection_server.set_offline();

			++it;
		}
	}

	void ServerCore::start_network_io_service()
	{
		_listen_sock.bind(server_info.ip, server_info.port);
		_listen_sock.listen();
		_listen_sock.accept(*io_accept_event.get());
		std::cout << "Listening to " << server_info.ip << ':' << server_info.port << "...\n";

		for (unsigned i = 0; i < server_info.num_of_event_threads; i++)
			io_service.spawn_event_loop_thread().detach();

		_state = ServerCore::State::Running;
	}

	/** 
	 *  Event handler interface
	 */

	void ServerCore::on_success(io::IoEvent* event)
	{
		// check there are expired tasks before accepting next client.
		interval_task_scheduler.process_tasks();

		try {
			_listen_sock.accept(*static_cast<io::IoAcceptEvent*>(event));
		}
		catch (...) {
			logging::cerr() << "fail to request accept";
			on_error(event);
		}
	}

	void ServerCore::on_error(io::IoEvent* event)
	{
		_state = ServerCore::State::Stopped;
	}

	std::optional<std::size_t> ServerCore::handle_io_event(io::EventType event_type, io::IoEvent* event)
	{
		assert(event_type == io::EventType::AcceptEvent);
		return handle_accept_event(*static_cast<io::IoAcceptEvent*>(event)) 
					? std::optional<std::size_t>(0) : std::nullopt;
	}

	bool ServerCore::handle_accept_event(io::IoAcceptEvent& event)
	{
		// creates unique accept socket first to avoid resource leak.
		auto client_socket = win::UniqueSocket(event.accepted_socket);

		if (connection_server_ids.size() > server_info.max_client_connections) {
			logging::cerr() << "full connection reached. skip to accept new client";
			return false;
		}

		// inherit the properties of the listen socket.
		win::Socket listen_sock = _listen_sock.get_handle();
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