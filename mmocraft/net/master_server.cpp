#include "pch.h"
#include "master_server.h"

namespace net
{
	MasterServer::MasterServer(std::string_view ip,
		int port,
		unsigned max_client_connections,
		unsigned num_of_event_threads,
		int concurrency_hint)
		: ServerCore{ ip, port, max_client_connections, num_of_event_threads, concurrency_hint }
	{ }

	bool MasterServer::handle_packet(ConnectionServer& connection_server, Packet* packet)
	{
		switch (packet->id) {
		case PacketID::Handshake:
			return handle_handshake_packet(connection_server, *static_cast<PacketHandshake*>(packet));
		default:
			return false;
		}
	}

	bool MasterServer::handle_handshake_packet(ConnectionServer& connection_server, PacketHandshake& packet)
	{
		std::cout << packet.username_and_host.size << ' ' << packet.username_and_host.data << '\n';
		return true;
	}
}