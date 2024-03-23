#pragma once

#include <string>
#include <memory>

#include "socket.h"
#include "io/io_service.h"
#include "config/config.h"
#include "util/noncopyable.h"

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
		using pointer = ServerCore*;

		ServerCore(std::string_view ip,
			int port,
			int concurrency_hint = io::DEFAULT_NUM_OF_CONCURRENT_EVENT_THREADS,
			int num_of_event_threads = io::DEFAULT_NUM_OF_READY_EVENT_THREADS);

		void serve_forever();

		win::Socket get_listen_socket() {
			return m_listen_sock.get_handle();
		}
		
	private:
		struct ServerCoreInfo
		{
			std::string_view ip;
			int port;
			int num_of_event_threads;
		} m_core_server_info;

		net::Socket m_listen_sock;
		io::IoCompletionPort m_io_service;
	};
}