#pragma once

#include <string>
#include <memory>

#include "socket.h"
#include "io/io_context.h"
#include "io/io_service.h"
#include "win/object_pool.h"
#include "util/common_util.h"
#include "config/config.h"

namespace
{
	class ServerCoreHandler
	{
	public:
		ServerCoreHandler() = delete;

		static void handle_accept(void *owner, io::IoContext *io_ctx, DWORD num_of_transferred_bytes, DWORD error_code);
	};
}

namespace net
{
	class ServerCore : util::NonCopyable, util::NonMovable
	{
	public:
		ServerCore(std::string_view ip,
			int port,
			unsigned max_client_connections,
			unsigned num_of_event_threads,
			int concurrency_hint = io::DEFAULT_NUM_OF_CONCURRENT_EVENT_THREADS);

		void create_client_session();
		
		void request_accept();

		void serve_forever();

		win::Socket get_listen_socket() {
			return m_listen_sock.get_handle();
		}
		
	private:
		const struct ServerInfo
		{
			std::string_view ip;
			int port;
			unsigned max_client_connections;
			unsigned num_of_event_threads;
		} m_server_info;

		net::Socket m_listen_sock;

		io::IoCompletionPort m_io_service;

		win::ObjectPool<io::IoContext> m_io_context_pool;

		win::ObjectPool<io::IoContext>::ObjectKey m_io_accept_ctx;
	};
}