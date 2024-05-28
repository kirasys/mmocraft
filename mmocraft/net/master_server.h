#pragma once

#include "database/database_core.h"

#include "net/connection_descriptor.h"
#include "net/server_core.h"
#include "net/packet.h"

namespace net
{
	class MasterServer : public ApplicationServer
	{
	public:
		MasterServer(std::string_view ip,
			int port,
			unsigned max_client_connections,
			unsigned num_of_event_threads,
			int concurrency_hint = io::DEFAULT_NUM_OF_CONCURRENT_EVENT_THREADS);

		bool handle_packet(ConnectionLevelDescriptor, Packet*, error::ErrorCode);

		bool handle_handshake_packet(ConnectionLevelDescriptor, PacketHandshake&);

		void serve_forever();

	private:
		net::ServerCore server_core;
		database::DatabaseCore database_core;

		WorkerLevelDescriptor worker_permission = WorkerLevelDescriptor(0);
	};
}