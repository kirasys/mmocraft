#include "pch.h"
#include "connection_server.h"

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

			if (auto connection_server = ConnectionServer::try_interact_with_client(event_owner)) {

				if (num_of_transferred_bytes == 0) { // client closes the socket.
					connection_server->set_offline();
					return;
				}

				if (not connection_server->dispatch_packets(num_of_transferred_bytes)) {
					// kick
				}

				connection_server->request_recv_client();
			}	
		}
	}

	ConnectionServer::ConnectionServer(win::UniqueSocket&& sock,
								ServerCore &main_server,
								io::IoCompletionPort& io_service,
								io::IoContextPool &io_context_pool)
		: m_client_socket{ std::move(sock) }
		, m_main_server{ main_server }
		, m_io_context_pool{ io_context_pool }
		, m_send_context{ io_context_pool.new_context<io::IoSendContext>(ServerHandler::handle_send) }
		, m_recv_context{ io_context_pool.new_context<io::IoRecvContext>(ServerHandler::handle_recv) }
	{
		m_connection_status.online = true;
		update_last_interaction_time();

		// allow to service client socket events.
		io_service.register_event_source(m_client_socket.get_handle(),
									/*.event_owner = */ reinterpret_cast<void*>(this));

		// init first recv.
		this->request_recv_client();
	}

	ConnectionServer::~ConnectionServer()
	{
		m_io_context_pool.delete_context(m_send_context);
		m_io_context_pool.delete_context(m_recv_context);
	}

	void ConnectionServer::request_recv_client()
	{
		m_client_socket.recv(*m_recv_context);
	}

	bool ConnectionServer::dispatch_packets(std::size_t num_of_received_bytes)
	{
		auto buf_start = m_recv_context->buffer;
		auto buf_end = buf_start + num_of_received_bytes + m_recv_context->num_of_unconsumed_bytes;
		auto packet_ptr = static_cast<Packet*>(_alloca(PacketStructure::size_of_max_packet_struct()));

		while (buf_start < buf_end) {
			const auto num_of_parsed_bytes = PacketStructure::parse_packet(buf_start, buf_end, packet_ptr);
			if (num_of_parsed_bytes == 0) // Insuffcient packet data
				break;

			if (not PacketStructure::validate_packet(packet_ptr)) {
				return false;
			}

			if (not m_main_server.handle_packet(*this, packet_ptr))
				logging::cerr() << "Unsupported packet id(" << packet_ptr->id << ")";

			buf_start += num_of_parsed_bytes;
		}

		assert((buf_end - buf_start < sizeof(m_recv_context->buffer)) && "Parsing error");
		m_recv_context->num_of_unconsumed_bytes = buf_end - buf_start;
		return true;
	}

	auto ConnectionServer::try_interact_with_client(void* server_instance) -> ConnectionServer*
	{
		auto connection_server = reinterpret_cast<ConnectionServer*>(server_instance);
		if (connection_server->is_online()) {
			connection_server->update_last_interaction_time();
			return connection_server;
		}
		return nullptr;
	}

	void ConnectionServer::set_offline()
	{
		// this lead to close the io completion port.
		m_client_socket.close();

		m_connection_status.online = false;
		m_connection_status.offline_time = util::current_timestmap();
	}

	bool ConnectionServer::is_expired(std::time_t current_time) const
	{
		return current_time >= m_connection_status.last_interaction_time + REQUIRED_SECONDS_FOR_EXPIRE;
	}

	bool ConnectionServer::is_safe_delete(std::time_t current_time) const
	{
		return not is_online()
			&& current_time >= m_connection_status.offline_time + REQUIRED_SECONDS_FOR_SECURE_DELETION;
	}
}