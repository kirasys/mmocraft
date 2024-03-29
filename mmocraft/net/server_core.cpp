#include "pch.h"
#include "server_core.h"

#include "logging/error.h"

namespace
{
	void ServerCoreHandler::handle_accept(void *event_owner, io::IoContext *io_ctx_ptr, DWORD num_of_transferred_bytes, DWORD error_code)
	{
		static_assert(std::is_same_v<io::IoContext::handler_type, decltype(&handle_accept)>, "Incorrect handler signature");
		
		if (error_code != ERROR_SUCCESS) {
			logging::cerr() << "handle_accept() failed with " << error_code;
			return;
		}

		auto &core_server = *static_cast<net::ServerCore*>(event_owner);
		auto &io_ctx  = *io_ctx_ptr;

		// inherit the properties of the listen socket.
		win::Socket listen_sock = core_server.get_listen_socket();
		if (SOCKET_ERROR == ::setsockopt(io_ctx.details.accept.accepted_socket,
						SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
						reinterpret_cast<char*>(&listen_sock), sizeof(listen_sock))) {
			logging::cerr() << "setsockopt(SO_UPDATE_ACCEPT_CONTEXT) failed";
			return;
		}

		// create client session.

		core_server.request_accept();
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
		, m_io_accept_ctx{ m_io_context_pool.new_object_safe() }
	{
		if (not m_io_accept_ctx.is_valid())
			return; // TODO: throw
		
		m_io_accept_ctx.get_object().handler = ServerCoreHandler::handle_accept;

		if (not m_io_service.register_event_source(m_listen_sock.get_handle(), /*.event_owner = */ this)) {
			std::cout << "fail to register handle" << std::endl;
		}

		m_listen_sock.bind(ip, port);
		m_listen_sock.listen();
	}

	void ServerCore::create_client_session()
	{

	}

	void ServerCore::request_accept()
	{
		auto &accept_ctx = m_io_accept_ctx.get_object();
		/*
		accept_ctx.overlapped.Internal = 0;
		accept_ctx.overlapped.InternalHigh = 0;
		accept_ctx.overlapped.Offset = 0;
		accept_ctx.overlapped.OffsetHigh = 0;
		accept_ctx.overlapped.hEvent = NULL;
		*/
		m_listen_sock.accept(accept_ctx);
	}

	void ServerCore::serve_forever()
	{
		this->request_accept();

		for (unsigned i = 0; i < m_server_info.num_of_event_threads; i++)
			m_io_service.spawn_event_loop_thread().detach();

		m_io_service.run_event_loop_forever();
	}
}