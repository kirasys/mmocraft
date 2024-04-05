#pragma once

#include <list>
#include <string>
#include <memory>

#include "socket.h"
#include "single_connection_server.h"
#include "io/io_context.h"
#include "io/io_service.h"
#include "win/object_pool.h"
#include "util/common_util.h"
#include "config/config.h"

#define DEFINE_HANDLER(x) static void x(void* event_owner, io::IoContext* io_ctx_ptr, DWORD num_of_transferred_bytes, DWORD error_code)

namespace
{
	using IoContextPool = win::ObjectPool<io::IoContext>;
	using ConnectionServerPool = win::ObjectPool<net::SingleConnectionServer>;
	using ConnectionServerID = ConnectionServerPool::ObjectID;
	using ConnectionServerScopedID = ConnectionServerPool::ScopedID;
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

		void serve_forever();

		bool try_accept();

		bool new_connection(win::UniqueSocket &&client_sock);

		bool delete_connection(ConnectionServerID);

		virtual void handle_packet(std::uint8_t buffer) = 0;

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

		IoContextPool m_io_context_pool;
		io::IoContext& m_accept_context;

		ConnectionServerPool m_single_connection_server_pool;
		std::list<ConnectionServerScopedID> m_online_connection_server_list;
	};
}