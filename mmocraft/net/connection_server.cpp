#include "pch.h"
#include "connection_server.h"

#include "logging/error.h"

namespace net
{
	ConnectionServer::ConnectionServer(ApplicationServer& a_app_server,
								win::UniqueSocket&& sock,
								io::IoCompletionPort& io_service,
								io::IoEventPool &io_event_pool)
		: descriptor_number{ ConnectionDescriptorTable::issue_descriptor_number() }
		, app_server{ a_app_server }
		, _client_socket{ std::move(sock) }
		, io_send_event_ptr{ io_event_pool.new_send_event() }
		, io_recv_event_ptr{ io_event_pool.new_recv_event() }
	{
		if (not is_valid())
			throw error::ErrorCode::CONNECTION_CREATE;

		connection_status.online = true;
		update_last_interaction_time();

		// allow to service client socket events.
		io_service.register_event_source(_client_socket.get_handle(), this);

		// register to the global descriptor table.
		ConnectionDescriptorTable::set_descriptor_data(descriptor_number,
			ConnectionDescriptorTable::DescriptorData{
				.connection = this,
				.raw_socket = _client_socket.get_handle(),
				.io_send_event = io_send_event_ptr.get(),
				.io_recv_event = io_recv_event_ptr.get(),
			}
		);

		// init first recv.
		ConnectionDescriptorTable::request_recv(descriptor_number);
	}

	ConnectionServer::~ConnectionServer()
	{
		
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

			if (not app_server.handle_packet(descriptor_number, packet_ptr))
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
		ConnectionDescriptorTable::delete_descriptor(descriptor_number);

		// this lead to close the io completion port.
		_client_socket.close();

		connection_status.online = false;
		connection_status.offline_time = util::current_timestmap();
	}

	bool ConnectionServer::is_expired(std::time_t current_time) const
	{
		return current_time >= connection_status.last_interaction_time + REQUIRED_SECONDS_FOR_EXPIRE;
	}

	bool ConnectionServer::is_safe_delete(std::time_t current_time) const
	{
		return not is_online()
			&& current_time >= connection_status.offline_time + REQUIRED_SECONDS_FOR_SECURE_DELETION;
	}

	/**
	 *  Event Handler Interface
	 */

	void ConnectionServer::on_success(io::IoEvent* event)
	{
		_client_socket.recv(*static_cast<io::IoRecvEvent*>(event));
	}

	void ConnectionServer::on_error(io::IoEvent* event)
	{
		if (not connection_status.online)
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

	/**
	 *  Connection Descriptor Table 
	 */

	unsigned ConnectionDescriptorTable::max_client_connections;
	unsigned ConnectionDescriptorTable::max_descriptor;
	std::unique_ptr<ConnectionDescriptorTable::DescriptorData[]> ConnectionDescriptorTable::descriptor_table;

	void ConnectionDescriptorTable::initialize(unsigned a_max_client_connections)
	{
		max_client_connections = a_max_client_connections;
		max_descriptor = 0;
		descriptor_table.reset(new DescriptorData[a_max_client_connections + 1]());
	}

	unsigned ConnectionDescriptorTable::issue_descriptor_number()
	{
		shrink_max_descriptor();

		for (unsigned i = 0; i < max_descriptor; i++) // find free slot.
			if (not descriptor_table[i].is_online) return i;

		max_descriptor += 1;
		assert(max_descriptor <= max_client_connections);
		return max_descriptor;
	}

	void ConnectionDescriptorTable::delete_descriptor(unsigned desc)
	{
		descriptor_table[desc].is_online = false;
	}

	void ConnectionDescriptorTable::shrink_max_descriptor()
	{
		for (unsigned i = max_descriptor; i > 0; i--) {
			if (descriptor_table[i].is_online) {
				max_descriptor = i;
				return;
			}
		}
		max_descriptor = 0;
	}

	void ConnectionDescriptorTable::set_descriptor_data(unsigned desc, DescriptorData data)
	{
		descriptor_table[desc] = data;
		std::atomic_thread_fence(std::memory_order_release);
		descriptor_table[desc].is_online = true;
	}

	void ConnectionDescriptorTable::request_recv(unsigned desc)
	{
		auto& data = descriptor_table[desc];
		if (data.is_online)
			Socket::recv(data.raw_socket, *data.io_recv_event);
	}
}