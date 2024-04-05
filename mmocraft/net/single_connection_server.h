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

		static SingleConnectionServer* try_interact_with_client(void* server_instance);

		bool is_online() const
		{
			return m_connection_status.online;
		}

		void set_offline()
		{
			m_connection_status.online = false;
		}

	private:
		net::Socket m_client_socket;

		ServerCore &m_main_server;

		IoContextPool::ScopedID m_send_context_id;
		IoContextPool::ScopedID m_recv_context_id;

		io::IoContext* const m_send_context;
		io::IoContext* const m_recv_context;

		struct ConnectionStatus {
			bool online	= false;
			std::time_t last_interaction_time = 0;
		} m_connection_status;

		void update_last_interaction_time()
		{
			m_connection_status.last_interaction_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		}
	};
}