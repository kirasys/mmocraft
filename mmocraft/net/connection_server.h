#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string_view>
#include <ctime>

#include "net/socket.h"
#include "net/application_server.h"
#include "io/io_event_pool.h"
#include "io/io_service.h"
#include "win/smart_handle.h"
#include "game/player.h"
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

		std::optional<std::size_t> process_packets(std::byte*, std::byte*);

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

		virtual void on_success(io::IoRecvEvent*) override;

		virtual void on_error(io::IoRecvEvent*) override;

		virtual std::optional<std::size_t> handle_io_event(io::IoRecvEvent*) override;

		virtual void on_success(io::IoSendEvent*) override;

		virtual void on_error(io::IoSendEvent*) override;

		//virtual std::optional<std::size_t> handle_io_event(io::IoSendEvent*) override;

		// connection register to the online descriptor table by this number.
		const unsigned descriptor_number;

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
	};

	class ConnectionDescriptorTable
	{
	public:
		struct DescriptorData
		{
			ConnectionServer* connection;
			win::Socket raw_socket;

			io::IoSendEventSmallData* io_send_event_small_data;
			io::IoSendEventData* io_send_event_data;
			io::IoSendEvent* io_send_event;
			io::IoRecvEvent* io_recv_event;

			bool is_online = false;
			bool is_send_event_running = false;
			bool is_recv_event_running = false;
		};

		ConnectionDescriptorTable() = delete;

		static void initialize(unsigned max_client_connections);

		static void request_recv_client_message(unsigned);

		static void request_send_server_message(unsigned);

		static bool push_server_message(unsigned, std::byte*, std::size_t);

		static bool push_short_server_message(unsigned, std::byte*, std::size_t);

		static void flush_server_message();

		static void flush_client_message();

		static bool push_disconnect_message(unsigned, std::string_view);

	private:
		// only connection server can modify descriptor table.
		friend ConnectionServer;

		static unsigned issue_descriptor_number();

		static void delete_descriptor(unsigned);

		static void set_descriptor_data(unsigned, DescriptorData);

		static void shrink_max_descriptor();

		static unsigned max_client_connections;
		static unsigned max_descriptor;
		static std::unique_ptr<DescriptorData[]> descriptor_table;
	};
}