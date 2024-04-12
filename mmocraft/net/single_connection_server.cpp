#include "pch.h"
#include "single_connection_server.h"

#include "packet.h"
#include "server_core.h"
#include "logging/error.h"

namespace net
{
	namespace ServerHandler
	{
		DEFINE_HANDLER(handle_send)
		{
			static_assert(std::is_same_v<io::IoContext::handler_type, decltype(&handle_send)>, "Incorrect handler signature");


		}

		DEFINE_HANDLER(handle_recv)
		{
			static_assert(std::is_same_v<io::IoContext::handler_type, decltype(&handle_recv)>, "Incorrect handler signature");

			if (auto connection_server = SingleConnectionServer::try_interact_with_client(event_owner)) {

				if (num_of_transferred_bytes == 0) { // client closes the socket.
					connection_server->set_offline();
					return;
				}

				if (auto remain_bytes = connection_server->dispatch_packets(num_of_transferred_bytes)) {

				}

				connection_server->request_recv_client();
			}	
		}
	}

	SingleConnectionServer::SingleConnectionServer(win::UniqueSocket&& sock,
								ServerCore &main_server,
								io::IoCompletionPort& io_service,
								IoContextPool &io_context_pool)
		: m_client_socket{ std::move(sock) }
		, m_main_server{ main_server }
		, m_send_context_id{ io_context_pool.new_object(ServerHandler::handle_send) }
		, m_recv_context_id{ io_context_pool.new_object(ServerHandler::handle_recv) }
		, m_send_context{ IoContextPool::find_object(m_send_context_id) }
		, m_recv_context{ IoContextPool::find_object(m_recv_context_id) }
	{
		m_connection_status.online = true;
		update_last_interaction_time();

		// allow to service client socket events.
		io_service.register_event_source(m_client_socket.get_handle(),
									/*.event_owner = */ reinterpret_cast<void*>(this));

		// init first recv.
		this->request_recv_client();
	}

	void SingleConnectionServer::request_recv_client()
	{
		m_client_socket.recv(*m_recv_context);
	}

	std::size_t SingleConnectionServer::dispatch_packets(std::size_t num_of_received_bytes)
	{
		auto buf_start = get_recv_buffer();
		auto buf_end = buf_start + num_of_received_bytes;
		auto packet_ptr = static_cast<Packet*>(_alloca(PacketStructure::size_of_max_packet_struct()));

		while (num_of_received_bytes > 0) {
			const auto num_of_parsed_bytes = PacketStructure::parse_packet(buf_start, buf_end, packet_ptr);
			if (num_of_parsed_bytes == 0) // Insuffcient packet data
				return num_of_received_bytes;

			if (not PacketStructure::validate_packet(packet_ptr)) {
				// kick
				return 0;
			}

			if (not m_main_server.handle_packet(*this, packet_ptr))
				logging::cerr() << "Unsupported packet id(" << packet_ptr->id << ")";

			num_of_received_bytes -= num_of_parsed_bytes;
		}
		assert(num_of_received_bytes == 0);
		return 0;
	}

	auto SingleConnectionServer::try_interact_with_client(void* server_instance) -> SingleConnectionServer*
	{
		auto connection_server = reinterpret_cast<SingleConnectionServer*>(server_instance);
		if (connection_server->is_online()) {
			connection_server->update_last_interaction_time();
			return connection_server;
		}
		return nullptr;
	}

	void SingleConnectionServer::set_offline()
	{
		// this lead to close the io completion port.
		m_client_socket.close();

		m_connection_status.online = false;
		m_connection_status.offline_time = util::current_timestmap();
	}

	bool SingleConnectionServer::is_expired(std::time_t current_time) const
	{
		return current_time >= m_connection_status.last_interaction_time + REQUIRED_SECONDS_FOR_EXPIRE;
	}

	bool SingleConnectionServer::is_safe_delete(std::time_t current_time) const
	{
		return not is_online()
			&& current_time >= m_connection_status.offline_time + REQUIRED_SECONDS_FOR_SECURE_DELETION;
	}
}