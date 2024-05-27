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
		: descriptor_number{ ConnectionDescriptorTable::issue_descriptor_number() }
		, app_server{ a_app_server }
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
		ConnectionDescriptorTable::set_descriptor_data(descriptor_number,
			ConnectionDescriptorTable::DescriptorData{
				.connection = this,
				.raw_socket = _client_socket.get_handle(),
				.io_send_event_small_data = io_send_event_small_data.get(),
				.io_send_event_data = io_send_event_data.get(),
				.io_send_event = io_send_event.get(),
				.io_recv_event = io_recv_event.get(),
			}
		);

		// init first recv.
		ConnectionDescriptorTable::request_recv_client_message(descriptor_number);
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
			const std::size_t parsed_bytes = PacketStructure::parse_packet(data_cur, data_end, packet_ptr);
			if (parsed_bytes == 0) // Insuffcient packet data
				break;

			if (not app_server.handle_packet(descriptor_number, packet_ptr))
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
		ConnectionDescriptorTable::delete_descriptor(descriptor_number);

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
		ConnectionDescriptorTable::request_recv_client_message(descriptor_number);
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
		ConnectionDescriptorTable::request_send_server_message(descriptor_number);
	}

	void ConnectionServer::on_error(io::IoSendEvent* event)
	{
		if (not connection_status.online)
			set_offline();
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

	void ConnectionDescriptorTable::request_recv_client_message(unsigned desc)
	{
		auto& desc_entry = descriptor_table[desc];
		if (not desc_entry.is_online)
			return;

		if (desc_entry.io_recv_event->data.unused_size() < PacketStructure::size_of_max_packet_struct())
			desc_entry.is_recv_event_running = false; return;

		WSABUF wbuf[1] = {};
		wbuf[0].buf = reinterpret_cast<char*>(desc_entry.io_recv_event->data.begin_unused());
		wbuf[0].len = ULONG(desc_entry.io_recv_event->data.unused_size());

		desc_entry.is_recv_event_running = Socket::recv(desc_entry.raw_socket, &desc_entry.io_recv_event->overlapped, wbuf, 1);
	}

	void ConnectionDescriptorTable::request_send_server_message(unsigned desc)
	{
		auto& desc_entry = descriptor_table[desc];
		if (not desc_entry.is_online) return;

		if (desc_entry.io_send_event_data->size() + desc_entry.io_send_event_small_data->size() == 0)
			desc_entry.is_send_event_running = false; return;

		WSABUF wbuf[2] = {};
		// send first short send buffer.
		desc_entry.io_send_event->transferred_small_data_bytes = desc_entry.io_send_event_small_data->size();
		wbuf[0].buf = reinterpret_cast<char*>(desc_entry.io_send_event_small_data->begin());
		wbuf[0].len = ULONG(desc_entry.io_send_event->transferred_small_data_bytes);

		wbuf[1].buf = reinterpret_cast<char*>(desc_entry.io_send_event_data->begin());
		wbuf[1].len = ULONG(desc_entry.io_send_event_data->size());

		desc_entry.is_send_event_running = Socket::send(desc_entry.raw_socket, &desc_entry.io_recv_event->overlapped, wbuf, 2);
	}

	bool ConnectionDescriptorTable::push_server_message(unsigned desc, std::byte* message, std::size_t n)
	{
		auto& desc_entry = descriptor_table[desc];
		return desc_entry.is_online ? desc_entry.io_send_event_data->push(message, n) : false;
	}

	bool ConnectionDescriptorTable::push_short_server_message(unsigned desc, std::byte* message, std::size_t n)
	{
		auto& desc_entry = descriptor_table[desc];
		return desc_entry.is_online ? desc_entry.io_send_event_small_data->push(message, n) : false;
	}

	void ConnectionDescriptorTable::flush_server_message()
	{
		for (unsigned desc = 0; desc < max_descriptor; ++desc) {
			auto& desc_entry = descriptor_table[desc];

			if (desc_entry.is_online && not desc_entry.is_send_event_running)
				request_send_server_message(desc);
		}
	}

	void ConnectionDescriptorTable::flush_client_message()
	{
		for (unsigned desc = 0; desc < max_descriptor; ++desc) {
			auto& desc_entry = descriptor_table[desc];

			if (desc_entry.is_online && not desc_entry.is_recv_event_running) {
				// if there are no unprocessed packets, connection will be close.
				// (because it is unusual situation)
				desc_entry.io_recv_event->invoke_handler(*desc_entry.connection, 
					desc_entry.io_recv_event->data.size() ? io::RETRY_SIGNAL : io::EOF_SIGNAL);
			}
		}
	}

	bool ConnectionDescriptorTable::push_disconnect_message(unsigned desc, std::string_view reason)
	{
		if (auto& desc_entry = descriptor_table[desc]; desc_entry.is_online) {
			net::PacketDisconnectPlayer disconnect_packet{ reason };
			return disconnect_packet.serialize(*desc_entry.io_send_event_small_data);
		}
		return false;
	}
}