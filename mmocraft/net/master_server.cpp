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

	bool MasterServer::handle_packet(unsigned connection_desc, Packet* packet)
	{
		switch (packet->id) {
		case PacketID::Handshake:
			return handle_handshake_packet(connection_desc, *static_cast<PacketHandshake*>(packet));
		default:
			return false;
		}
	}

	bool MasterServer::handle_handshake_packet(unsigned connection_desc, PacketHandshake& packet)
	{
		if (const char* spliter = std::strchr(packet.username_and_host.data, ';')) {
			//connection_server
			return true;
		}

		return false;
	}

	void MasterServer::serve_forever()
	{
		server_core.serve_forever();
	}
}