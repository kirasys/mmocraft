#include "pch.h"
#include "server_core.h"

#include "logging/error.h"

namespace net
{
	void ServerCoreHandler::handle_accept(void* event_owner, io::IoContext* io_ctx_ptr, DWORD num_of_transferred_bytes, DWORD error_code)
	{
		static_assert(std::is_same_v<io::IoContext::handler_type, decltype(&handle_accept)>, "Incorrect handler signature");
		
		auto &server = *static_cast<net::ServerCore*>(event_owner);

		util::defer accept_again = [&server] {
			server.try_accept();
		};

		if (error_code != ERROR_SUCCESS) {
			logging::cerr() << "handle_accept() failed with " << error_code;
			return;
		}

		auto &io_ctx = *io_ctx_ptr;
		auto client_socket = io_ctx.details.accept.accepted_socket;

		// inherit the properties of the listen socket.
		win::Socket listen_sock = server.get_listen_socket();
		if (SOCKET_ERROR == ::setsockopt(client_socket,
						SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
						reinterpret_cast<char*>(&listen_sock), sizeof(listen_sock))) {
			logging::cerr() << "setsockopt(SO_UPDATE_ACCEPT_CONTEXT) failed";
			return;
		}

		// add a client to the server.
		auto client_id = server.new_connection(client_socket);
		if (not client_id) {
			logging::cerr() << "fail to create a client";
			return;
		}

		auto connection_server = ConnectionServerPool::find_object(client_id);
		if (connection_server->request_recv_client()) {
			logging::cerr() << "fail to client first recv()";
			return;
		}
	}

	void ServerCoreHandler::handle_send(void* event_owner, io::IoContext* io_ctx_ptr, DWORD num_of_transferred_bytes, DWORD error_code)
	{
		static_assert(std::is_same_v<io::IoContext::handler_type, decltype(&handle_send)>, "Incorrect handler signature");
		

	}

	void ServerCoreHandler::handle_recv(void* event_owner, io::IoContext* io_ctx_ptr, DWORD num_of_transferred_bytes, DWORD error_code)
	{
		static_assert(std::is_same_v<io::IoContext::handler_type, decltype(&handle_recv)>, "Incorrect handler signature");
		
		auto connection_server_id = reinterpret_cast<ConnectionServerID>(event_owner);
		std::cout << io_ctx_ptr->details.recv.buffer << '\n';

		auto connection_server = ConnectionServerPool::find_object(connection_server_id);
		if (connection_server->request_recv_client()) {
			logging::cerr() << "fail to receive data from the client";
			return;
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
								m_io_context_pool.new_object_unsafe(ServerCoreHandler::handle_accept)) }
		, m_single_connection_server_pool{ max_client_connections }
	{	
		if (not m_io_service.register_event_source(m_listen_sock.get_handle(), /*.event_owner = */ this))
			return;

		m_listen_sock.bind(ip, port);
		m_listen_sock.listen();
		std::cout << "Listening to " << ip << ':' << port << "...\n";
	}

	auto ServerCore::new_connection(win::Socket client_sock) -> ConnectionServerID
	{
		// create io contexts(overlapped structures) for send/recv.
		auto client_send_context_id = m_io_context_pool.new_object(ServerCoreHandler::handle_send);
		auto client_recv_context_id = m_io_context_pool.new_object(ServerCoreHandler::handle_recv);
		if (not client_send_context_id.is_valid() || not client_recv_context_id.is_valid())
			return { 0 };

		// create a user (server manages life cycle of the client session)
		auto connection_server_id = m_single_connection_server_pool.new_object_unsafe(
			client_sock,
			/* server = */ *this,
			std::move(client_send_context_id),
			std::move(client_recv_context_id)
		);

		if (not connection_server_id)
			return { 0 };

		// allow to service client socket events.
		if (not m_io_service.register_event_source(client_sock, 
										/*.event_owner = */ reinterpret_cast<void*>(connection_server_id)))
			return { 0 };

		return connection_server_id;
	}

	bool ServerCore::delete_connection(ConnectionServerID connection_server_id)
	{
		return ConnectionServerPool::free_object(connection_server_id);
	}

	void ServerCore::accept()
	{
		m_accept_context.overlapped.Internal = 0;
		m_accept_context.overlapped.InternalHigh = 0;
		m_accept_context.overlapped.Offset = 0;
		m_accept_context.overlapped.OffsetHigh = 0;
		m_accept_context.overlapped.hEvent = NULL;

		m_listen_sock.accept(m_accept_context);
	}

	void ServerCore::try_accept()
	{
		try {
			this->accept();
		}
		catch (const error::NetworkException& code) {
			std::cerr << code.what() << std::endl;
		}
	}

	void ServerCore::serve_forever()
	{
		this->accept();

		for (unsigned i = 0; i < m_server_info.num_of_event_threads; i++)
			m_io_service.spawn_event_loop_thread().detach();

		m_io_service.run_event_loop_forever();
	}
}