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
		, io_accept_event_data { io_event_pool.new_accept_event_data() }
		, io_accept_event { io_event_pool.new_accept_event(io_accept_event_data.get()) }
		, connection_server_pool{ max_client_connections }
		, interval_task_scheduler{ this }
	{	
		io_service.register_event_source(_listen_sock.get_handle(), /*.event_handler = */ this);

		ConnectionDescriptor::initialize(max_client_connections);

		//
		// Schedule interval tasks.
		// 
		interval_task_scheduler.schedule("keep-alive", &ServerCore::check_connection_expiration, util::MilliSecond(10000));

		_state = ServerCore::State::Initialized;
	}

	void ServerCore::new_connection(win::UniqueSocket &&client_sock)
	{
		// create a server for single client.
		auto connection_server_ptr = connection_server_pool.new_object(
			app_server,
			std::move(client_sock),
			io_service,
			io_event_pool
		);

		connection_server_ptrs.emplace_back(std::move(connection_server_ptr));
	}

	void ServerCore::check_connection_expiration()
	{
		auto current_tick = util::current_monotonic_tick();

		for (auto it = connection_server_ptrs.begin(); it != connection_server_ptrs.end();) {
			auto &connection_server = *(*it).get();

			if (connection_server.is_safe_delete(current_tick)) {
				it = connection_server_ptrs.erase(it);
				continue;
			}

			if (connection_server.is_expired(current_tick))
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

	bool ServerCore::post_server_event(ServerEventType type, ServerEventHandler* event_handler)
	{
		return io_service.push_event(type, event_handler, nullptr);
	}

	/** 
	 *  Event handler interface
	 */

	void ServerCore::on_complete(io::IoAcceptEvent* event)
	{
		// check there are expired tasks before accepting next client.
		interval_task_scheduler.process_tasks();

		if (event->result != error::SUCCESS)
			_state = ServerCore::State::Stopped; return;

		try {
			_listen_sock.accept(*event);
		}
		catch (...) {
			logging::cerr() << "fail to request accept";
			_state = ServerCore::State::Stopped;
		}
	}

	std::size_t ServerCore::handle_io_event(io::IoAcceptEvent* event)
	{
		// creates unique accept socket first to avoid resource leak.
		auto client_socket = win::UniqueSocket(event->accepted_socket);

		if (connection_server_ptrs.size() > server_info.max_client_connections) {
			logging::cerr() << "full connection reached. skip to accept new client";
			event->result = error::CLIENT_CONNECTION_FULL;
			return 0;
		}

		// inherit the properties of the listen socket.
		win::Socket listen_sock = _listen_sock.get_handle();
		if (SOCKET_ERROR == ::setsockopt(client_socket,
			SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
			reinterpret_cast<char*>(&listen_sock), sizeof(listen_sock))) {
			logging::cerr() << "setsockopt(SO_UPDATE_ACCEPT_CONTEXT) failed";
			event->result = error::SOCKET_SETOPT;
			return 0;
		}

		// add a client to the server.
		new_connection(std::move(client_socket));
		event->result = error::SUCCESS;
		return 0;
	}
}