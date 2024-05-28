#include "pch.h"
#include "connection_server.h"

#include "logging/error.h"
#include "net/packet.h"

namespace net
{
	ConnectionServer::ConnectionServer(ApplicationServer& a_app_server,
								win::UniqueSocket&& sock,
								io::IoCompletionPort& io_service,
								io::IoEventPool &io_event_pool)
		: app_server{ a_app_server }
		, _client_socket{ std::move(sock) }

		, io_send_event_small_data{ io_event_pool.new_send_event_small_data() }
		, io_send_event_data{ io_event_pool.new_send_event_data() }
		, io_send_event{ io_event_pool.new_send_event(io_send_event_data.get(), io_send_event_small_data.get()) }

		, io_recv_event_data{ io_event_pool.new_recv_event_data() }
		, io_recv_event{ io_event_pool.new_recv_event(io_recv_event_data.get()) }
	{
		if (not is_valid())
			throw error::ErrorCode::CONNECTION_CREATE;

		connection_status.online = true;
		update_last_interaction_time();

		// allow to service client socket events.
		io_service.register_event_source(_client_socket.get_handle(), this);

		// register to the global descriptor table.
		if (not ConnectionDescriptor::issue_descriptor_number(descriptor_number))
			throw error::ErrorCode::CONNECTION_CREATE;

		ConnectionDescriptor::set_descriptor_data(descriptor_number,
			ConnectionDescriptor::DescriptorData{
				.connection = this,
				.raw_socket = _client_socket.get_handle(),
				.io_send_event_small_data = io_send_event_small_data.get(),
				.io_send_event_data = io_send_event_data.get(),
				.io_send_event = io_send_event.get(),
				.io_recv_event = io_recv_event.get(),
			}
		);

		// init first recv.
		ConnectionDescriptor::activate_receive_cycle(descriptor_number);
	}

	ConnectionServer::~ConnectionServer()
	{
		
	}

	void ConnectionServer::request_send()
	{

	}

	std::optional<std::size_t> ConnectionServer::process_packets(std::byte* data_begin, std::byte* data_end)
	{
		auto data_cur = data_begin;
		auto packet_ptr = static_cast<Packet*>(_alloca(PacketStructure::size_of_max_packet_struct()));

		while (data_cur < data_end) {
			auto [parsed_bytes, error_code] = PacketStructure::parse_packet(data_cur, data_end, packet_ptr);
			if (error_code == error::PACKET_INSUFFIENT_DATA)
				break;

			if (error_code != error::SUCCESS) {
				// kick
				return;
			}

			if (not app_server.handle_packet(ConnectionLevelDescriptor(descriptor_number), packet_ptr))
				break; // Couldn't handle right now. try again later.

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
		ConnectionDescriptor::delete_descriptor(descriptor_number);

		// this lead to close the io completion port.
		_client_socket.close();

		connection_status.online = false;
		connection_status.offline_tick = util::current_monotonic_tick();
	}

	bool ConnectionServer::is_expired(std::size_t current_tick) const
	{
		return current_tick >= connection_status.last_interaction_tick + REQUIRED_MILLISECONDS_FOR_EXPIRE;
	}

	bool ConnectionServer::is_safe_delete(std::size_t current_tick) const
	{
		return not is_online()
			&& current_tick >= connection_status.offline_tick + REQUIRED_MILLISECONDS_FOR_SECURE_DELETION;
	}

	/**
	 *  Event Handler Interface
	 */
	
	/// recv event handler

	void ConnectionServer::on_success(io::IoRecvEvent* event)
	{
		ConnectionDescriptor::activate_receive_cycle(descriptor_number);
	}

	void ConnectionServer::on_error(io::IoRecvEvent* event)
	{
		if (not connection_status.online)
			set_offline();
	}

	std::optional<std::size_t> ConnectionServer::handle_io_event(io::IoRecvEvent* event)
	{
		if (not try_interact_with_client())
			return std::nullopt; // timeout case: connection will be deleted soon.

		return process_packets(event->data.begin(), event->data.end());
	}

	/// send event handler

	void ConnectionServer::on_success(io::IoSendEvent* event)
	{
		ConnectionDescriptor::activate_send_cycle(descriptor_number);
	}

	void ConnectionServer::on_error(io::IoSendEvent* event)
	{
		if (not connection_status.online)
			set_offline();
	}
}