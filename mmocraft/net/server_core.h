#pragma once

#include <list>
#include <vector>
#include <string>
#include <memory>

#include "socket.h"
#include "packet.h"
#include "connection_server.h"
#include "io/io_event_pool.h"
#include "io/io_service.h"
#include "win/object_pool.h"
#include "util/common_util.h"
#include "util/interval_task.h"
#include "config/config.h"

namespace
{
	using ConnectionServerPool = win::ObjectPool<net::ConnectionServer>;
}

namespace net
{
	class ServerCore : io::IoEventHandler, util::NonCopyable, util::NonMovable
	{
	public:
		ServerCore(std::string_view ip,
			int port,
			unsigned max_client_connections,
			unsigned num_of_event_threads,
			int concurrency_hint = io::DEFAULT_NUM_OF_CONCURRENT_EVENT_THREADS);

		void serve_forever();

		unsigned issue_online_connection_key();

		void delete_online_connection_key(unsigned);

		void new_connection(win::UniqueSocket &&client_sock);

		void check_connection_expiration();

		virtual bool handle_packet(ConnectionServer&, Packet*) = 0;

		/* Event handler interface */

		virtual void on_success(io::IoEvent*) override;

		virtual void on_error(io::IoEvent*) override;

		virtual std::optional<std::size_t> handle_io_event(io::EventType, io::IoEvent*) override;

		bool handle_accept_event(io::IoAcceptEvent&);
		
	private:
		void shrink_max_online_connection_key();

		const struct ServerInfo
		{
			std::string_view ip;
			int port;
			unsigned max_client_connections;
			unsigned num_of_event_threads;
		} m_server_info;

		net::Socket m_listen_sock;

		io::IoCompletionPort m_io_service;

		io::IoEventPool& m_io_event_pool;
		io::IoAcceptEventPtr m_accept_event;

		ConnectionServerPool m_connection_server_pool;
		std::list<ConnectionServerPool::ScopedID> m_connection_list;

		unsigned max_online_connection_key;
		std::unique_ptr<ConnectionServer*[]> online_connection_table;

		util::IntervalTaskScheduler<ServerCore> m_interval_task_scheduler;
	};
}