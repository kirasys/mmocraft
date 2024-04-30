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
								io::IoEventPool &io_event_pool)
		: m_client_socket{ std::move(sock) }
		, m_main_server{ main_server }
		, m_io_event_pool{ io_event_pool }
		, m_send_event{ io_event_pool.new_send_event() }
		, m_recv_event{ io_event_pool.new_recv_event() }
	{
		m_connection_status.online = true;
		update_last_interaction_time();

		// allow to service client socket events.
		io_service.register_event_source(m_client_socket.get_handle(), this);

		// init first recv.
		this->request_recv();
	}

	ConnectionServer::~ConnectionServer()
	{
		m_io_event_pool.delete_event(m_send_event);
		m_io_event_pool.delete_event(m_recv_event);
	}

	void ConnectionServer::request_recv()
	{
		m_client_socket.recv(*m_recv_event);
	}

	void ConnectionServer::request_send()
	{

	}

	std::optional<std::size_t> ConnectionServer::process_packets(std::uint8_t* data_begin, std::uint8_t* data_end)
	{
		auto data_cur = data_begin;
		auto packet_ptr = static_cast<Packet*>(_alloca(PacketStructure::size_of_max_packet_struct()));

		while (data_cur < data_end) {
			const std::size_t parsed_bytes = PacketStructure::parse_packet(data_cur, data_end, packet_ptr);
			if (parsed_bytes == 0) // Insuffcient packet data
				break;

			if (not PacketStructure::validate_packet(packet_ptr)) {
				return std::nullopt;
			}

			if (not m_main_server.handle_packet(*this, packet_ptr))
				logging::cerr() << "Unsupported packet id(" << packet_ptr->id << ")";

			data_cur += parsed_bytes;
		}

		assert(data_cur <= data_end && "Parsing error");
		return data_cur - data_begin; // num of total parsed bytes.
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
		request_recv();
	}

	void ConnectionServer::on_error()
	{
		if (not m_connection_status.online)
			set_offline();
	}

	std::optional<std::size_t> ConnectionServer::handle_io_event(io::EventType event_type, io::IoEvent* event)
	{
		switch (event_type)
		{
		case io::EventType::RecvEvent: return handle_recv_event(*static_cast<io::IoRecvEvent*>(event));
		case io::EventType::SendEvent: return handle_send_event(*static_cast<io::IoSendEvent*>(event));
		default: assert(false);
		}
		return std::nullopt;
	}

	std::optional<std::size_t> ConnectionServer::handle_recv_event(io::IoRecvEvent& event)
	{
		if (not try_interact_with_client())
			return std::nullopt; // timeout case: connection will be deleted soon.

		return process_packets(event.data.begin(), event.data.end());
	}

	std::optional<std::size_t> ConnectionServer::handle_send_event(io::IoSendEvent& event)
	{
		if (not is_online())
			return std::nullopt;



		return 1;
	}
}