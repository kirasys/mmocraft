#include "pch.h"
#include "master_server.h"

#include <cstring>

#include "config/config.h"
#include "logging/error.h"

namespace net
{
	MasterServer::MasterServer(std::string_view ip,
		int port,
		unsigned max_client_connections,
		unsigned num_of_event_threads,
		int concurrency_hint)
		: server_core{ *this, ip, port, max_client_connections, num_of_event_threads, concurrency_hint }
		, database_core{ }
	{ 
		const auto& conf = config::get_config();

		if (not database_core.connect_with_password(conf.db))
			throw error::DATABASE_CONNECT;
	}

	error::ErrorCode MasterServer::handle_packet(ConnectionLevelDescriptor conn_descriptor, Packet* packet)
	{
		switch (packet->id) {
		case PacketID::Handshake:
			return handle_handshake_packet(conn_descriptor, *static_cast<PacketHandshake*>(packet));
		default:
			return error::PACKET_UNIMPLEMENTED_ID;
		}
	}

	error::ErrorCode MasterServer::handle_handshake_packet(ConnectionLevelDescriptor conn_descriptor, PacketHandshake& packet)
	{
		deferred_packet_stack.push<net::PacketHandshake>(conn_descriptor, packet);
		return error::PACKET_HANDLE_DEFERRED;
	}

	void MasterServer::serve_forever()
	{
		server_core.start_network_io_service();

		while (1) {
			std::size_t start_tick = util::current_monotonic_tick();

			ConnectionDescriptor::flush_server_message(worker_permission);
			ConnectionDescriptor::flush_client_message(worker_permission);

			std::size_t end_tick = util::current_monotonic_tick();

			if (auto diff = end_tick - start_tick; diff < 1000)
				util::sleep_ms(std::max(1000 - diff, std::size_t(100)));
		}
	}

	bool MasterServer::post_deferrend_packet_event(IDeferredPacketEvent* event)
	{
		return server_core.post_io_event(event, this);
	}

	/**
	 *  Event handler interface
	 */

	void MasterServer::handle_deferred_packet(DeferredPacketEvent<PacketHandshake>* event)
	{
		for (auto defer_packet = event->head; defer_packet; defer_packet = defer_packet->next) {
			std::cout << defer_packet->connection_descriptor << ' '
				<< defer_packet->_username << '\n';
		}
	}
}