#include "pch.h"
#include "server_core.h"

#include <cassert>

#include "logging/error.h"

namespace net
{
	std::list<win::ObjectPool<net::ConnectionServer>::Pointer> ServerCore::connection_server_ptrs;

	ServerCore::ServerCore(PacketHandleServer& a_packet_handle_server, const config::Configuration& conf)
		: packet_handle_server{ a_packet_handle_server }
		, server_info{ .ip = conf.server.ip,
						 .port = conf.server.port, 
						 .max_client_connections = conf.server.max_player,
						 .num_of_event_threads = conf.system.num_of_processors * 2}
		, _listen_sock{ net::SocketType::TCPv4 }
		, io_service{ io::DEFAULT_NUM_OF_CONCURRENT_EVENT_THREADS }
		, io_event_pool{ conf.server.max_player }
		, io_accept_event_data { io_event_pool.new_accept_event_data() }
		, io_accept_event { io_event_pool.new_accept_event(io_accept_event_data.get()) }
		, connection_server_pool{ conf.server.max_player }
	{	
		io_service.register_event_source(_listen_sock.get_handle(), /*.event_handler = */ this);

		_state = ServerCore::State::Initialized;
	}

	void ServerCore::new_connection(win::UniqueSocket &&client_sock)
	{
		// create a server for single client.
		auto connection_server_ptr = connection_server_pool.new_object(
			packet_handle_server,
			std::move(client_sock),
			io_service,
			io_event_pool
		);

		connection_server_ptrs.emplace_back(std::move(connection_server_ptr));
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

	bool ServerCore::post_event(PacketEvent* event, DeferredPacketHandler* event_handler)
	{
		return io_service.push_event(event, event_handler);
	}

	void ServerCore::cleanup_expired_connection()
	{
		for (auto it = connection_server_ptrs.begin(); it != connection_server_ptrs.end();) {
			auto& connection_server = *(*it).get();

			if (connection_server.is_safe_delete()) {
				it = connection_server_ptrs.erase(it);
				continue;
			}

			if (connection_server.is_expired())
				connection_server.set_offline();

			++it;
		}
	}

	/** 
	 *  Event handler interface
	 */

	void ServerCore::on_complete(io::IoAcceptEvent* event)
	{
		if (not last_error_code.is_strong_success()) {
			LOG(error) << last_error_code;
			_state = ServerCore::State::Stopped;
			return;
		}

		if (auto error_code = _listen_sock.accept(*event)) {
			LOG(error) << "fail to request accept with " << error_code;
			_state = ServerCore::State::Stopped;
		}
	}

	std::size_t ServerCore::handle_io_event(io::IoAcceptEvent* event)
	{
		// creates unique accept socket first to avoid resource leak.
		auto client_socket = win::UniqueSocket(event->accepted_socket);

		if (connection_server_ptrs.size() > server_info.max_client_connections) {
			last_error_code = error::CLIENT_CONNECTION_FULL;
			return 0;
		}

		// inherit the properties of the listen socket.
		win::Socket listen_sock = _listen_sock.get_handle();
		if (SOCKET_ERROR == ::setsockopt(client_socket,
			SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
			reinterpret_cast<char*>(&listen_sock), sizeof(listen_sock))) {
			last_error_code = error::SOCKET_SETOPT;
			return 0;
		}

		// add a client to the server.
		new_connection(std::move(client_socket));
		last_error_code = error::SUCCESS;
		return 0;
	}
}