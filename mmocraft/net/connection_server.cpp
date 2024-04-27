#include "pch.h"
#include "connection_server.h"

#include "packet.h"
#include "server_core.h"
#include "logging/error.h"

namespace net
{
	ConnectionServer::ConnectionServer(win::UniqueSocket&& sock,
								ServerCore &main_server,
								io::IoCompletionPort& io_service,
								io::IoContextPool &io_context_pool)
		: m_client_socket{ std::move(sock) }
		, m_main_server{ main_server }
		, m_io_context_pool{ io_context_pool }
		, m_send_context{ io_context_pool.new_context<io::IoSendContext>() }
		, m_recv_context{ io_context_pool.new_context<io::IoRecvContext>() }
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

	std::optional<std::size_t> ConnectionServer::process_packets()
	{
		auto buf_begin = m_recv_context->buffer_begin(), buf_end = m_recv_context->buffer_end();
		auto packet_ptr = static_cast<Packet*>(_alloca(PacketStructure::size_of_max_packet_struct()));

		while (buf_begin < buf_end) {
			const std::size_t parsed_bytes = PacketStructure::parse_packet(buf_begin, buf_end, packet_ptr);
			if (parsed_bytes == 0) // Insuffcient packet data
				break;

			if (not PacketStructure::validate_packet(packet_ptr)) {
				return std::nullopt;
			}

			if (not m_main_server.handle_packet(*this, packet_ptr))
				logging::cerr() << "Unsupported packet id(" << packet_ptr->id << ")";

			buf_begin += parsed_bytes;
		}

		assert(buf_begin <= buf_end && "Parsing error");
		return buf_begin - m_recv_context->buffer_begin(); // num of total parsed bytes.
	}

	bool ConnectionServer::try_interact_with_client()
	{
		if (is_online()) {
			update_last_interaction_time();
			return true;
		}
		return false;
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

	/* Event Handler Interface */

	void ConnectionServer::on_success()
	{
		request_recv_client();
	}

	void ConnectionServer::on_error()
	{
		if (not m_connection_status.online)
			set_offline();
	}

	std::optional<std::size_t> ConnectionServer::handle_io_event(io::EventType event_type)
	{
		switch (event_type)
		{
		case io::EventType::RecvEvent: return handle_recv_event();
		case io::EventType::SendEvent: return handle_send_event();
		default: assert(false);
		}
		return std::nullopt;
	}

	std::optional<std::size_t> ConnectionServer::handle_recv_event()
	{
		if (not try_interact_with_client())
			return std::nullopt; // timeout case: connection will be deleted soon.

		return process_packets();
	}

	std::optional<std::size_t> ConnectionServer::handle_send_event()
	{
		return 1;
	}
}