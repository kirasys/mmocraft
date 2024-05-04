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

		unsigned issue_connection_descriptor();

		void delete_connection_descriptor(unsigned);

		void new_connection(win::UniqueSocket &&client_sock);

		void check_connection_expiration();

		virtual bool handle_packet(ConnectionServer&, Packet*) = 0;

		/* Event handler interface */

		virtual void on_success(io::IoEvent*) override;

		virtual void on_error(io::IoEvent*) override;

		virtual std::optional<std::size_t> handle_io_event(io::EventType, io::IoEvent*) override;

		bool handle_accept_event(io::IoAcceptEvent&);
		
	private:
		void shrink_max_connection_descriptor();

		const struct ServerInfo
		{
			std::string_view ip;
			int port;
			unsigned max_client_connections;
			unsigned num_of_event_threads;
		} server_info;

		net::Socket _listen_sock;

		io::IoCompletionPort io_service;

		io::IoEventPool& io_event_pool;
		io::IoAcceptEventPtr io_accept_event;

		ConnectionServerPool connection_server_pool;
		std::list<ConnectionServerPool::ScopedID> connection_server_ids;

		unsigned max_connection_descriptor;
		std::unique_ptr<ConnectionServer*[]> connection_descriptor_table;

		util::IntervalTaskScheduler<ServerCore> interval_task_scheduler;
	};
}