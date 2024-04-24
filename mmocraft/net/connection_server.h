#pragma once

#include <memory>
#include <chrono>
#include <ctime>

#include "io/io_context_pool.h"
#include "io/io_service.h"
#include "net/socket.h"
#include "win/smart_handle.h"
#include "util/common_util.h"

namespace net
{
	class ServerCore;

	class ConnectionServer : util::NonCopyable, util::NonMovable
	{
		// The minecrft beta server will disconnect a client,
		// if it doesn't receive at least one packet before 1200 in-game ticks (1200 tick = 60s)
		static constexpr unsigned REQUIRED_SECONDS_FOR_EXPIRE = 60;
		static constexpr unsigned REQUIRED_SECONDS_FOR_SECURE_DELETION = 5;

	public:
		ConnectionServer(win::UniqueSocket&&, ServerCore&, io::IoCompletionPort& , io::IoContextPool&);

		~ConnectionServer();

		bool is_valid() const
		{
			return m_send_context != nullptr && m_recv_context != nullptr;
		}

		void request_recv_client();
		
		//void send_to_client();

		bool dispatch_packets(std::size_t num_of_received_bytes);

		/* Methods related to connection status */

		static ConnectionServer* try_interact_with_client(void* server_instance);
		
		void update_last_interaction_time(std::time_t current_time = util::current_timestmap())
		{
			m_connection_status.last_interaction_time = current_time;
		}

		void set_offline();

		bool is_online() const
		{
			return m_connection_status.online;
		}

		bool is_expired(std::time_t current_time = util::current_timestmap()) const;

		bool is_safe_delete(std::time_t current_time = util::current_timestmap()) const;

	private:
		net::Socket m_client_socket;

		ServerCore &m_main_server;

		io::IoContextPool& m_io_context_pool;
		io::IoSendContext* const m_send_context;
		io::IoRecvContext* const m_recv_context;

		struct ConnectionStatus {
			bool online	= false;
			std::time_t offline_time = 0;
			std::time_t last_interaction_time = 0;
		} m_connection_status;
	};
}