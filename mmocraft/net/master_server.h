#pragma once
#include "server_core.h"
#include "packet.h"

namespace net
{
	class MasterServer : public ServerCore
	{
	public:
		MasterServer(std::string_view ip,
			int port,
			unsigned max_client_connections,
			unsigned num_of_event_threads,
			int concurrency_hint = io::DEFAULT_NUM_OF_CONCURRENT_EVENT_THREADS);

		bool handle_packet(ConnectionServer&, Packet*) override;

		bool handle_handshake_packet(ConnectionServer&, PacketHandshake&);
	};
}