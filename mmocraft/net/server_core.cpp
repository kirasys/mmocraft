#include "pch.h"
#include "server_core.h"

#include "logging/error.h"

#define DEFINE_HANDLER(x) static void x(void* event_owner, io::IoContext* io_ctx_ptr, DWORD num_of_transferred_bytes, DWORD error_code)

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

			util::defer accept_next_client = [&server] {
				if (not server.try_accept()) {
					// TODO: accept retry
					logging::cerr() << "fail to accept again";
				}
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
		, m_single_connection_server_pool{ max_client_connections }
	{	
		m_io_service.register_event_source(m_listen_sock.get_handle(), /*.event_owner = */ this);

		m_listen_sock.bind(ip, port);
		m_listen_sock.listen();
		std::cout << "Listening to " << ip << ':' << port << "...\n";
	}

	bool ServerCore::new_connection(win::UniqueSocket &&client_sock)
	{
		// create a server for single server.
		auto connection_server_id = m_single_connection_server_pool.new_object(
			std::move(client_sock),
			/* main_server = */ *this,
			m_io_service,
			m_io_context_pool
		);

		auto connection_server = ConnectionServerPool::find_object(connection_server_id);
		if (not connection_server || not connection_server->is_valid())
			return false;

		connection_server_id.release();

		return true;
	}

	bool ServerCore::delete_connection(ConnectionServerID connection_server_id)
	{
		// TODO: this is not a thread-safe!
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

	bool ServerCore::try_accept()
	{
		try {
			this->accept();
			return true;
		}
		catch (...) {
			return false;
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