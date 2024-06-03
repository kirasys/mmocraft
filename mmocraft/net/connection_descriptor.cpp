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
	std::unordered_map<game::PlayerID, game::Player*> ConnectionDescriptor::player_lookup_table;

	void ConnectionDescriptor::initialize_system()
	{
		const auto& conf = config::get_config();

		descriptor_end = 0;
		descriptor_table_capacity = conf.server.max_player;
		descriptor_table.reset(new DescriptorData[descriptor_table_capacity]());

		player_lookup_table.reserve(descriptor_table_capacity);
	}

	bool ConnectionDescriptor::issue_descriptor_number(DescriptorType::Connection& desc)
	{
		shrink_descriptor_end();

		for (unsigned i = 0; i < descriptor_end; i++) { // find free slot.
			if (not descriptor_table[i].is_online) {
				desc = DescriptorType::Connection(i); return true;
			}
		}

		if (descriptor_end >= descriptor_table_capacity)
			return false;

		desc = DescriptorType::Connection(descriptor_end++);
		return true;
	}

	void ConnectionDescriptor::set_descriptor_data(DescriptorType::Connection desc, DescriptorData data)
	{
		descriptor_table[desc] = data;
		std::atomic_thread_fence(std::memory_order_release);
		descriptor_table[desc].is_online = true;
	}

	void ConnectionDescriptor::delete_descriptor(DescriptorType::Connection desc)
	{
		auto& desc_entry = descriptor_table[desc];
		desc_entry.is_online = false;

		// clean player lookup table.
		player_lookup_table.erase(desc_entry.player->get_identity_number());
	}

	void ConnectionDescriptor::shrink_descriptor_end()
	{
		for (unsigned i = descriptor_end; i > 0; i--) {
			if (descriptor_table[i - 1].is_online) {
				descriptor_end = i; return;
			}
		}

		descriptor_end = 0;
	}

	void ConnectionDescriptor::activate_receive_cycle(DescriptorType::Connection desc)
	{
		auto& desc_entry = descriptor_table[desc];
		if (not desc_entry.is_online)
			return;

		if (desc_entry.io_recv_event->data.unused_size() < PacketStructure::max_size_of_packet_struct()) {
			desc_entry.is_recv_event_running = false;
			return;
		}

		WSABUF wbuf[1] = {};
		wbuf[0].buf = reinterpret_cast<char*>(desc_entry.io_recv_event->data.begin_unused());
		wbuf[0].len = ULONG(desc_entry.io_recv_event->data.unused_size());

		desc_entry.is_recv_event_running = Socket::recv(desc_entry.raw_socket, &desc_entry.io_recv_event->overlapped, wbuf, 1);
	}

	void ConnectionDescriptor::activate_send_cycle(DescriptorType::Connection desc)
	{
		auto& desc_entry = descriptor_table[desc];
		if (not desc_entry.is_online) return;

		if (desc_entry.io_send_event_data->size() + desc_entry.io_send_event_small_data->size() == 0) {
			desc_entry.is_send_event_running = false;
			return;
		}

		WSABUF wbuf[2] = {};
		// send first short send buffer.
		wbuf[0].buf = reinterpret_cast<char*>(desc_entry.io_send_event_small_data->begin());
		wbuf[0].len = ULONG(desc_entry.io_send_event_small_data->size());

		wbuf[1].buf = reinterpret_cast<char*>(desc_entry.io_send_event_data->begin());
		wbuf[1].len = ULONG(desc_entry.io_send_event_data->size());

		desc_entry.is_send_event_running = Socket::send(desc_entry.raw_socket, &desc_entry.io_recv_event->overlapped, wbuf, 2);
	}

	void ConnectionDescriptor::flush_server_message(DescriptorType::Tick)
	{
		for (unsigned desc = 0; desc < descriptor_end; ++desc) {
			auto& desc_entry = descriptor_table[desc];

			if (desc_entry.is_online && not desc_entry.is_send_event_running)
				activate_send_cycle(DescriptorType::Connection(desc));
		}
	}

	void ConnectionDescriptor::flush_client_message(DescriptorType::Tick)
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

	bool ConnectionDescriptor::associate_game_player
		(DescriptorType::DeferredPacket desc, game::PlayerID player_id, game::PlayerType player_type, const char* username, const char* password)
	{
		auto& desc_entry = descriptor_table[desc];
		if (not desc_entry.is_online)
			return false;

		if (player_lookup_table.find(player_id) != player_lookup_table.end())
			return false; // already associated.

		auto player_ptr = std::make_unique<game::Player>(
			player_id,
			player_type,
			username,
			password
		);

		desc_entry.player = player_ptr.get();
		desc_entry.connection->set_player(std::move(player_ptr)); // transfer ownership to tje connection server.

		player_lookup_table[player_id] = desc_entry.player;
		return true;
	}
}