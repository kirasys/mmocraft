#include "pch.h"
#include "server_core.h"

#include "logging/error.h"

namespace
{
	void ServerCoreHandler::handle_accept(void *event_owner, io::IoContext *io_ctx_ptr, DWORD num_of_transferred_bytes, DWORD error_code)
	{
		static_assert(std::is_same_v<io::IoContext::handler_type, decltype(&handle_accept)>, "Incorrect handler signature");
		
		auto& core_server = *static_cast<net::ServerCore*>(event_owner);

		util::defer accept_again = [&core_server] {
			core_server.try_accept();
		};

		if (error_code != ERROR_SUCCESS) {
			logging::cerr() << "handle_accept() failed with " << error_code;
			return;
		}

		auto& io_ctx = *io_ctx_ptr;

		// inherit the properties of the listen socket.
		win::Socket listen_sock = core_server.get_listen_socket();
		if (SOCKET_ERROR == ::setsockopt(io_ctx.details.accept.accepted_socket,
						SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
						reinterpret_cast<char*>(&listen_sock), sizeof(listen_sock))) {
			logging::cerr() << "setsockopt(SO_UPDATE_ACCEPT_CONTEXT) failed";
			return;
		}

		// create client session.
		if (not core_server.create_client_session(io_ctx.details.accept.accepted_socket)) {
			logging::cerr() << "fail to create_client_session";
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
		, m_accept_ctx_key{ m_io_context_pool.new_object_safe() }
		, m_accept_ctx{ m_accept_ctx_key.get_object() }
		, m_client_session_pool{ max_client_connections }
	{	
		if (not m_io_service.register_event_source(m_listen_sock.get_handle(), /*.event_owner = */ this))
			return;

		m_listen_sock.bind(ip, port);
		m_listen_sock.listen();

		m_accept_ctx.handler = ServerCoreHandler::handle_accept;
	}

	bool ServerCore::create_client_session(win::Socket client_sock)
	{
		auto client_session_key = m_client_session_pool.new_object_safe(
			client_sock,
			m_io_context_pool.new_object_safe(),
			m_io_context_pool.new_object_safe()
		);

		if (not client_session_key.is_valid())
			return false;

		if (not m_io_service.register_event_source(client_sock,
													/*.event_owner = */ &client_session_key.get_object())) {
			return false;
		}

		return true;
	}

	void ServerCore::try_accept()
	{	
		m_accept_ctx.overlapped.Internal = 0;
		m_accept_ctx.overlapped.InternalHigh = 0;
		m_accept_ctx.overlapped.Offset = 0;
		m_accept_ctx.overlapped.OffsetHigh = 0;
		m_accept_ctx.overlapped.hEvent = NULL;
		
		m_listen_sock.accept(m_accept_ctx);
	}

	void ServerCore::serve_forever()
	{
		this->try_accept();

		for (unsigned i = 0; i < m_server_info.num_of_event_threads; i++)
			m_io_service.spawn_event_loop_thread().detach();

		m_io_service.run_event_loop_forever();
	}
}