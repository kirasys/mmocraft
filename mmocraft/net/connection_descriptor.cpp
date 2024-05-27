#include "pch.h"

#include "connection_descriptor.h"

#include "net/connection_server.h"
#include "net/socket.h"
#include "net/packet.h"

namespace net
{
	unsigned ConnectionDescriptorTable::max_client_connections;
	unsigned ConnectionDescriptorTable::max_descriptor;
	std::unique_ptr<ConnectionDescriptorTable::DescriptorData[]> ConnectionDescriptorTable::descriptor_table;

	void ConnectionDescriptorTable::initialize(unsigned a_max_client_connections)
	{
		max_client_connections = a_max_client_connections;
		max_descriptor = 0;
		descriptor_table.reset(new DescriptorData[a_max_client_connections + 1]());
	}

	bool ConnectionDescriptorTable::issue_descriptor_number(AdminLevelDescriptor& desc)
	{
		shrink_max_descriptor();

		for (unsigned i = 0; i < max_descriptor; i++) // find free slot.
			if (not descriptor_table[i].is_online) desc = AdminLevelDescriptor(i); return true;

		if (max_descriptor >= max_client_connections)
			return false;

		desc = AdminLevelDescriptor(max_descriptor++);
		return true;
	}

	void ConnectionDescriptorTable::delete_descriptor(AdminLevelDescriptor desc)
	{
		descriptor_table[desc].is_online = false;
	}

	void ConnectionDescriptorTable::shrink_max_descriptor()
	{
		for (unsigned i = max_descriptor; i > 0; i--)
			if (descriptor_table[i].is_online)
				max_descriptor = i; return;
		
		max_descriptor = 0;
	}

	void ConnectionDescriptorTable::set_descriptor_data(AdminLevelDescriptor desc, DescriptorData data)
	{
		descriptor_table[desc] = data;
		std::atomic_thread_fence(std::memory_order_release);
		descriptor_table[desc].is_online = true;
	}

	void ConnectionDescriptorTable::activate_receive_cycle(AdminLevelDescriptor desc)
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

	void ConnectionDescriptorTable::activate_send_cycle(AdminLevelDescriptor desc)
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

	bool ConnectionDescriptorTable::push_server_message(WorkerLevelDescriptor desc, std::byte* message, std::size_t n)
	{
		auto& desc_entry = descriptor_table[desc];
		return desc_entry.is_online ? desc_entry.io_send_event_data->push(message, n) : false;
	}

	bool ConnectionDescriptorTable::push_server_message(ConnectionLevelDescriptor desc, std::byte* message, std::size_t n)
	{
		auto& desc_entry = descriptor_table[desc];
		return desc_entry.is_online ? desc_entry.io_send_event_small_data->push(message, n) : false;
	}

	void ConnectionDescriptorTable::flush_server_message()
	{
		for (unsigned desc = 0; desc < max_descriptor; ++desc) {
			auto& desc_entry = descriptor_table[desc];

			if (desc_entry.is_online && not desc_entry.is_send_event_running)
				activate_send_cycle(AdminLevelDescriptor(desc));
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

	bool ConnectionDescriptorTable::push_disconnect_message(ConnectionLevelDescriptor desc, std::string_view reason)
	{
		if (auto& desc_entry = descriptor_table[desc]; desc_entry.is_online) {
			net::PacketDisconnectPlayer disconnect_packet{ reason };
			return disconnect_packet.serialize(*desc_entry.io_send_event_small_data);
		}
		return false;
	}
}