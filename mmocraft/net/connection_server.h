#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string_view>
#include <ctime>

#include "game/player.h"
#include "io/io_event_pool.h"
#include "io/io_service.h"
#include "net/socket.h"
#include "net/application_server.h"
#include "net/connection_descriptor.h"
#include "win/smart_handle.h"
#include "util/common_util.h"

namespace net
{
	class ConnectionServer : public io::IoEventHandler, util::NonCopyable, util::NonMovable
	{
		// The minecrft beta server will disconnect a client,
		// if it doesn't receive at least one packet before 1200 in-game ticks (1200 tick = 60s)
		static constexpr unsigned REQUIRED_MILLISECONDS_FOR_EXPIRE = 60 * 1000;
		static constexpr unsigned REQUIRED_MILLISECONDS_FOR_SECURE_DELETION = 5 * 1000;

	public:
		enum State
		{
			Initialized,
			
		};

		ConnectionServer(ApplicationServer&, win::UniqueSocket&&, io::IoCompletionPort& , io::IoEventPool&);

		~ConnectionServer();

		bool is_valid() const
		{
			return io_send_event && io_recv_event;
		}
		
		void request_send();

		auto process_packets(std::byte*, std::byte*) -> std::pair<std::uint32_t, error::ErrorCode>;

		void set_player(std::unique_ptr<game::Player>&&);

		/**
		 *  Methods related to connection status
		 */

		bool try_interact_with_client();
		
		void update_last_interaction_time(std::size_t current_tick = util::current_monotonic_tick())
		{
			connection_status.last_interaction_tick = current_tick;
		}

		void set_offline();

		bool is_online() const
		{
			return connection_status.online;
		}

		bool is_expired(std::size_t current_tick = util::current_monotonic_tick()) const;

		bool is_safe_delete(std::size_t current_tick = util::current_monotonic_tick()) const;

		/**
		 *  Event Handler Interface
		 */

		virtual void on_complete(io::IoRecvEvent*) override;

		virtual std::size_t handle_io_event(io::IoRecvEvent*) override;

		virtual void on_complete(io::IoSendEvent*) override;

		//virtual std::optional<std::size_t> handle_io_event(io::IoSendEvent*) override;

		// connection register to the online descriptor table by this number.
		DescriptorType::Connection descriptor_number;

	private:
		ApplicationServer& app_server;

		net::Socket _client_socket;

		win::ObjectPool<io::IoSendEventSmallData>::Pointer io_send_event_small_data;
		win::ObjectPool<io::IoSendEventData>::Pointer io_send_event_data;
		win::ObjectPool<io::IoSendEvent>::Pointer io_send_event;

		win::ObjectPool<io::IoRecvEventData>::Pointer io_recv_event_data;
		win::ObjectPool<io::IoRecvEvent>::Pointer io_recv_event;

		struct ConnectionStatus {
			bool online	= false;
			std::size_t offline_tick = 0;
			std::size_t last_interaction_tick = 0;
		} connection_status;

		std::unique_ptr<game::Player> _player;
	};
}