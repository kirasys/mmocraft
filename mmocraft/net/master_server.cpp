#include "pch.h"

#include <cstring>

#include "master_server.h"

namespace net
{
	MasterServer::MasterServer(std::string_view ip,
		int port,
		unsigned max_client_connections,
		unsigned num_of_event_threads,
		int concurrency_hint)
		: server_core{ *this, ip, port, max_client_connections, num_of_event_threads, concurrency_hint }
	{ }

	bool MasterServer::handle_packet(unsigned conn_descriptor, Packet* packet)
	{
		switch (packet->id) {
		case PacketID::Handshake:
			return handle_handshake_packet(conn_descriptor, *static_cast<PacketHandshake*>(packet));
		default:
			return false;
		}
	}

	bool MasterServer::handle_handshake_packet(unsigned conn_descriptor, PacketHandshake& packet)
	{
		// TODO: online authentication.
		std::byte message[] = { std::byte('-') };
		ConnectionDescriptorTable::push_short_server_message(conn_descriptor, message, sizeof(message));
		return true;
	}

	void MasterServer::serve_forever()
	{
		server_core.start_network_io_service();

		while (1) {
			std::size_t start_tick = util::current_monotonic_tick();

			ConnectionDescriptorTable::flush_server_message();
			ConnectionDescriptorTable::flush_client_message();

			std::size_t end_tick = util::current_monotonic_tick();

			if (auto diff = end_tick - start_tick; diff < 1000)
				::Sleep(std::max(1000 - diff, std::size_t(100)));
		}
	}
}