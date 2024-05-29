#pragma once

#include <list>
#include <vector>
#include <string>
#include <memory>

#include "net/socket.h"
#include "net/packet.h"
#include "net/connection_server.h"
#include "net/application_server.h"
#include "net/server_event.h"
#include "io/io_event_pool.h"
#include "io/io_service.h"
#include "win/object_pool.h"
#include "util/common_util.h"
#include "util/interval_task.h"
#include "config/config.h"

namespace net
{
	class ServerCore final : io::IoEventHandler
	{
	public:
		enum State
		{
			Uninitialized,
			Initialized,
			Running,
			Stopped,
		};

		ServerCore(ApplicationServer&,
			std::string_view ip,
			int port,
			unsigned max_client_connections,
			unsigned num_of_event_threads,
			int concurrency_hint = io::DEFAULT_NUM_OF_CONCURRENT_EVENT_THREADS);

		ServerCore::State status() const
		{
			return _state;
		}

		void start_network_io_service();

		bool post_server_event(ServerEventType, ServerEventHandler*);

		void new_connection(win::UniqueSocket &&client_sock);

		void check_connection_expiration();

		/**
		 *  Event handler interface 
		 */

		virtual void on_complete(io::IoAcceptEvent*) override;

		virtual std::size_t handle_io_event(io::IoAcceptEvent*) override;
		
	private:
		ServerCore::State _state = Uninitialized;

		ApplicationServer& app_server;

		const struct ServerInfo
		{
			std::string_view ip;
			int port;
			unsigned max_client_connections;
			unsigned num_of_event_threads;
		} server_info;

		net::Socket _listen_sock;

		io::IoCompletionPort io_service;

		io::IoEventPool io_event_pool;
		win::ObjectPool<io::IoAcceptEventData>::Pointer io_accept_event_data;
		win::ObjectPool<io::IoAcceptEvent>::Pointer io_accept_event;

		win::ObjectPool<net::ConnectionServer> connection_server_pool;
		std::list<win::ObjectPool<net::ConnectionServer>::Pointer> connection_server_ptrs;

		util::IntervalTaskScheduler<ServerCore> interval_task_scheduler;
	};
}