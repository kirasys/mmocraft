#pragma once

#include <list>
#include <string>
#include <memory>

#include "socket.h"
#include "packet.h"
#include "connection_server.h"
#include "io/io_context_pool.h"
#include "io/io_service.h"
#include "win/object_pool.h"
#include "util/common_util.h"
#include "util/interval_task.h"
#include "config/config.h"

#define DEFINE_HANDLER(x) static void x(void* event_owner, io::IoContext* io_ctx_ptr, DWORD num_of_transferred_bytes, DWORD error_code)

namespace
{
	using ConnectionServerPool = win::ObjectPool<net::ConnectionServer>;
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

		void accept_next_client();

		bool new_connection(win::UniqueSocket &&client_sock);

		void check_connection_expiration();

		virtual bool handle_packet(ConnectionServer&, Packet*) = 0;

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

		io::IoContextPool m_io_context_pool;
		io::IoAcceptContext* const m_accept_context; // don't have to delete manually

		ConnectionServerPool m_connection_server_pool;
		std::list<ConnectionServer*> m_connection_list;

		util::IntervalTaskScheduler<ServerCore> m_interval_task_scheduler;
	};
}