#include "pch.h"

#include "connection_descriptor.h"

#include "net/connection_server.h"
#include "net/socket.h"
#include "net/packet.h"

namespace net
{
	unsigned ConnectionDescriptor::descriptor_table_capacity;
	unsigned ConnectionDescriptor::descriptor_end;
	std::unique_ptr<ConnectionDescriptor::DescriptorData[]> ConnectionDescriptor::descriptor_table;

	void ConnectionDescriptor::initialize(unsigned a_max_client_connections)
	{
		descriptor_end = 0;
		descriptor_table_capacity = a_max_client_connections;
		descriptor_table.reset(new DescriptorData[a_max_client_connections]());

		player_lookup_table.reserve(a_max_client_connections);
	}

	bool ConnectionDescriptor::issue_descriptor_number(AdminLevelDescriptor& desc)
	{
		shrink_descriptor_end();

		for (unsigned i = 0; i < descriptor_end; i++) // find free slot.
			if (not descriptor_table[i].is_online) {
				desc = AdminLevelDescriptor(i); return true;
			}

		if (descriptor_end >= descriptor_table_capacity)
			return false;

		desc = AdminLevelDescriptor(descriptor_end++);
		return true;
	}

	void ConnectionDescriptor::delete_descriptor(AdminLevelDescriptor desc)
	{
		descriptor_table[desc].is_online = false;
	}

	void ConnectionDescriptor::shrink_descriptor_end()
	{
		for (unsigned i = descriptor_end; i > 0; i--)
			if (descriptor_table[i - 1].is_online) {
				descriptor_end = i; return;
			}

		descriptor_end = 0;
	}

	void ConnectionDescriptor::set_descriptor_data(AdminLevelDescriptor desc, DescriptorData data)
	{
		descriptor_table[desc] = data;
		std::atomic_thread_fence(std::memory_order_release);
		descriptor_table[desc].is_online = true;
	}

	void ConnectionDescriptor::activate_receive_cycle(AdminLevelDescriptor desc)
	{
		auto& desc_entry = descriptor_table[desc];
		if (not desc_entry.is_online)
			return;

		if (desc_entry.io_recv_event->data.unused_size() < PacketStructure::size_of_max_packet_struct()) {
			desc_entry.is_recv_event_running = false; return;
		}

		WSABUF wbuf[1] = {};
		wbuf[0].buf = reinterpret_cast<char*>(desc_entry.io_recv_event->data.begin_unused());
		wbuf[0].len = ULONG(desc_entry.io_recv_event->data.unused_size());

		desc_entry.is_recv_event_running = Socket::recv(desc_entry.raw_socket, &desc_entry.io_recv_event->overlapped, wbuf, 1);
	}

	void ConnectionDescriptor::activate_send_cycle(AdminLevelDescriptor desc)
	{
		auto& desc_entry = descriptor_table[desc];
		if (not desc_entry.is_online) return;

		if (desc_entry.io_send_event_data->size() + desc_entry.io_send_event_small_data->size() == 0) {
			desc_entry.is_send_event_running = false; return;
		}

		WSABUF wbuf[2] = {};
		// send first short send buffer.
		desc_entry.io_send_event->transferred_small_data_bytes = desc_entry.io_send_event_small_data->size();
		wbuf[0].buf = reinterpret_cast<char*>(desc_entry.io_send_event_small_data->begin());
		wbuf[0].len = ULONG(desc_entry.io_send_event->transferred_small_data_bytes);

		wbuf[1].buf = reinterpret_cast<char*>(desc_entry.io_send_event_data->begin());
		wbuf[1].len = ULONG(desc_entry.io_send_event_data->size());

		desc_entry.is_send_event_running = Socket::send(desc_entry.raw_socket, &desc_entry.io_recv_event->overlapped, wbuf, 2);
	}

	void ConnectionDescriptor::flush_server_message(WorkerLevelDescriptor)
	{
		for (unsigned desc = 0; desc < descriptor_end; ++desc) {
			auto& desc_entry = descriptor_table[desc];

			if (desc_entry.is_online && not desc_entry.is_send_event_running)
				activate_send_cycle(AdminLevelDescriptor(desc));
		}
	}

	void ConnectionDescriptor::flush_client_message(WorkerLevelDescriptor)
	{
		for (unsigned desc = 0; desc < descriptor_end; ++desc) {
			auto& desc_entry = descriptor_table[desc];

			if (desc_entry.is_online && not desc_entry.is_recv_event_running) {
				// if there are no unprocessed packets, connection will be close.
				// (because it is unusual situation)
				desc_entry.io_recv_event->invoke_handler(*desc_entry.connection,
					desc_entry.io_recv_event->data.size() ? io::RETRY_SIGNAL : io::EOF_SIGNAL);
			}
		}
	}

	bool ConnectionDescriptor::push_server_message(WorkerLevelDescriptor desc, std::byte* message, std::size_t n)
	{
		auto& desc_entry = descriptor_table[desc];
		return desc_entry.is_online ? desc_entry.io_send_event_data->push(message, n) : false;
	}

	bool ConnectionDescriptor::push_server_message(ConnectionLevelDescriptor desc, std::byte* message, std::size_t n)
	{
		auto& desc_entry = descriptor_table[desc];
		return desc_entry.is_online ? desc_entry.io_send_event_small_data->push(message, n) : false;
	}

	bool ConnectionDescriptor::push_disconnect_message(ConnectionLevelDescriptor desc, std::string_view reason)
	{
		if (auto& desc_entry = descriptor_table[desc]; desc_entry.is_online) {
			desc_entry.connection->set_offline();

			net::PacketDisconnectPlayer disconnect_packet{ reason };
			return disconnect_packet.serialize(*desc_entry.io_send_event_small_data);
		}
		return false;
	}

	bool ConnectionDescriptor::on_login_complete
		(ConnectionLevelDescriptor desc, game::PlayerID player_id, game::PlayerType player_type, const char* username, const char* password)
	{
		auto& desc_entry = descriptor_table[desc];
		if (not desc_entry.is_online) return;

		auto player_ptr = std::make_unique<game::Player>(
			player_id,
			player_type,
			username,
			password
		);

		desc_entry.player = player_ptr.get();
		desc_entry.connection->set_player(std::move(player_ptr)); // transfer ownership to tje connection server.
		
		player_lookup_table.insert({ player_id, desc_entry.player });
	}

	bool ConnectionDescriptor::push_disconnect_message(AdminLevelDescriptor desc, std::string_view reason)
	{
		return push_disconnect_message(ConnectionLevelDescriptor(desc), reason);
	}
}