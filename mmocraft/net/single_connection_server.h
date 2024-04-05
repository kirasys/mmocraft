#pragma once

#include <memory>
#include <chrono>
#include <ctime>

#include "io/io_service.h"
#include "net/socket.h"
#include "win/object_pool.h"
#include "win/smart_handle.h"
#include "util/common_util.h"

namespace net
{
	class ServerCore;

	class SingleConnectionServer : util::NonCopyable, util::NonMovable
	{
		using IoContextPool = win::ObjectPool<io::IoContext>;
		
		static constexpr unsigned REQUIRED_SECONDS_FOR_EXPIRE = 5 * 60;
		static constexpr unsigned REQUIRED_SECONDS_FOR_SECURE_DELETION = 5;

	public:
		SingleConnectionServer(win::UniqueSocket&&, ServerCore&, io::IoCompletionPort& , IoContextPool&);

		bool is_valid() const
		{
			return m_send_context != nullptr && m_recv_context != nullptr;
		}

		auto get_recv_buffer() const
		{
			return m_recv_context->details.recv.buffer;
		}

		void request_recv_client();
		
		//void send_to_client();

		/* Methods related to connection status */

		static SingleConnectionServer* try_interact_with_client(void* server_instance);
		
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

		IoContextPool::ScopedID m_send_context_id;
		IoContextPool::ScopedID m_recv_context_id;

		io::IoContext* const m_send_context;
		io::IoContext* const m_recv_context;

		struct ConnectionStatus {
			bool online	= false;
			std::time_t offline_time = 0;
			std::time_t last_interaction_time = 0;
		} m_connection_status;
	};
}