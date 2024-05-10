#pragma once

#include <atomic>
#include <memory>
#include <chrono>
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
	class ConnectionServer : io::IoEventHandler, util::NonCopyable, util::NonMovable
	{
		// The minecrft beta server will disconnect a client,
		// if it doesn't receive at least one packet before 1200 in-game ticks (1200 tick = 60s)
		static constexpr unsigned REQUIRED_SECONDS_FOR_EXPIRE = 60;
		static constexpr unsigned REQUIRED_SECONDS_FOR_SECURE_DELETION = 5;

	public:
		ConnectionServer(ApplicationServer&, win::UniqueSocket&&, io::IoCompletionPort& , io::IoEventPool&);

		~ConnectionServer();

		bool is_valid() const
		{
			return io_send_event_ptr.is_valid() && io_recv_event_ptr.is_valid();
		}
		
		void request_send();

		std::optional<std::size_t> process_packets(std::byte*, std::byte*);

		/**
		 *  Methods related to connection status
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

		/**
		 *  Event Handler Interface
		 */

		virtual void on_success(io::IoEvent*) override;

		virtual void on_error(io::IoEvent*) override;

		virtual std::optional<std::size_t> handle_io_event(io::EventType, io::IoEvent*) override;

		std::optional<std::size_t> handle_recv_event(io::IoRecvEvent&);

		std::optional<std::size_t> handle_send_event(io::IoSendEvent&);

		// connection register to the online descriptor table by this number.
		const unsigned descriptor_number;

	private:
		ApplicationServer& app_server;

		net::Socket _client_socket;

		io::IoSendEventPtr io_send_event_ptr;
		io::IoRecvEventPtr io_recv_event_ptr;

		struct ConnectionStatus {
			bool online	= false;
			std::time_t offline_time = 0;
			std::time_t last_interaction_time = 0;
		} connection_status;
	};

	class ConnectionDescriptorTable
	{
	public:
		struct DescriptorData
		{
			bool is_online = false;
			ConnectionServer* connection;
			win::Socket raw_socket;
			io::IoSendEvent* io_send_event;
			io::IoSendEventData* io_send_data = nullptr;
			io::IoRecvEvent* io_recv_event;
		};

		ConnectionDescriptorTable() = delete;

		static void initialize(unsigned max_client_connections);

		static void request_recv_client_message(unsigned);

		static bool push_server_message(unsigned, std::byte*, std::size_t);

		static bool push_server_short_message(unsigned, std::byte*, std::size_t);

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