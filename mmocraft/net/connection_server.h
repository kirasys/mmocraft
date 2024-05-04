#pragma once

#include <memory>
#include <chrono>
#include <ctime>


#include "net/socket.h"
#include "io/io_event_pool.h"
#include "io/io_service.h"
#include "win/smart_handle.h"
#include "game/player.h"
#include "util/common_util.h"

namespace net
{
	class ServerCore;

	class ConnectionServer : io::IoEventHandler, util::NonCopyable, util::NonMovable
	{
		// The minecrft beta server will disconnect a client,
		// if it doesn't receive at least one packet before 1200 in-game ticks (1200 tick = 60s)
		static constexpr unsigned REQUIRED_SECONDS_FOR_EXPIRE = 60;
		static constexpr unsigned REQUIRED_SECONDS_FOR_SECURE_DELETION = 5;

	public:
		ConnectionServer(win::UniqueSocket&&, ServerCore&, io::IoCompletionPort& , io::IoEventPool&);

		~ConnectionServer();

		bool is_valid() const
		{
			return io_send_event.is_valid() && io_recv_event.is_valid();
		}

		//void request_recv();
		
		void request_send();

		std::optional<std::size_t> process_packets(std::uint8_t*, std::uint8_t*);

		/**
		 * Methods related to connection status
		 */

		bool try_interact_with_client();
		
		void update_last_interaction_time(std::time_t current_time = util::current_timestmap())
		{
			connection_status.last_interaction_time = current_time;
		}

		void set_offline();

		bool is_online() const
		{
			return connection_status.online;
		}

		bool is_expired(std::time_t current_time = util::current_timestmap()) const;

		bool is_safe_delete(std::time_t current_time = util::current_timestmap()) const;

		/* Event Handler Interface */

		virtual void on_success(io::IoEvent*) override;

		virtual void on_error(io::IoEvent*) override;

		virtual std::optional<std::size_t> handle_io_event(io::EventType, io::IoEvent*) override;

		std::optional<std::size_t> handle_recv_event(io::IoRecvEvent&);

		std::optional<std::size_t> handle_send_event(io::IoSendEvent&);

		// main server maintains online connection table through this key.
		const unsigned descriptor_number;

	private:
		net::Socket _client_socket;

		ServerCore &main_server;

		io::IoSendEventPtr io_send_event;
		io::IoRecvEventPtr io_recv_event;

		struct ConnectionStatus {
			bool online	= false;
			std::time_t offline_time = 0;
			std::time_t last_interaction_time = 0;
		} connection_status;
	};
}