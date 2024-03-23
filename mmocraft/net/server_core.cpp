#include "server_core.h"

#include "../logging/error.h"

namespace
{
	void ServerCoreHandler::handle_accept(void *event_owner, io::IoContext *io_ctx, DWORD num_of_transferred_bytes, DWORD error_code)
	{
		static_assert(std::is_same_v<io::IoContext::handler_type, decltype(&handle_accept)>);
		
		if (error_code != ERROR_SUCCESS) {
			logging::cerr() << "handle_accept() failed with " << error_code;
			return;
		}

		auto &core_server = *static_cast<net::ServerCore::pointer>(event_owner);
		auto &accept_ctx  = *static_cast<io::AcceptIoContext*>(io_ctx);

		// inherit the properties of the listen socket
		win::Socket listen_sock = core_server.get_listen_socket();
		if (SOCKET_ERROR == ::setsockopt(accept_ctx.accepted_socket,
						SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
						reinterpret_cast<char*>(&listen_sock), sizeof(listen_sock))) {
			logging::cerr() << "setsockopt(SO_UPDATE_ACCEPT_CONTEXT) failed";
			return;
		}
	}
}

namespace net
{
	ServerCore::ServerCore(std::string_view ip, int port, int concurrency_hint, int num_of_event_threads) 
		: m_core_server_info{ .ip = ip, .port = port, .num_of_event_threads = num_of_event_threads }
		, m_listen_sock{ net::SocketType::TCPv4 }
		, m_io_service(concurrency_hint)
	{
		if (num_of_event_threads == io::DEFAULT_NUM_OF_READY_EVENT_THREADS) {
			decltype(auto) conf = config::get_config();
			m_core_server_info.num_of_event_threads = conf.system.num_of_processors * 2;
		}

		if (not m_io_service.register_event_source(m_listen_sock.get_handle(), /*.owner = */ this)) {
			std::cout << "fail to register handle" << std::endl;
		}

		m_listen_sock.bind(m_core_server_info.ip, m_core_server_info.port);
		m_listen_sock.listen();
	}

	void ServerCore::serve_forever()
	{
		io::AcceptIoContext accept_ctx;
		accept_ctx.handler = ServerCoreHandler::handle_accept;

		m_listen_sock.accept(accept_ctx);

		for (int i = 0; i < m_core_server_info.num_of_event_threads; i++)
			m_io_service.spawn_event_loop_thread().detach();

		m_io_service.run_event_loop_forever();
	}
}