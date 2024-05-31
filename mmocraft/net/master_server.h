#pragma once

#include "database/database_core.h"

#include "net/connection_descriptor.h"
#include "net/deferred_packet.h"
#include "net/server_core.h"
#include "net/packet.h"

namespace net
{
	class MasterServer : public ApplicationServer, public net::DeferredPacketHandler
	{
	public:
		MasterServer(std::string_view ip,
			int port,
			unsigned max_client_connections,
			unsigned num_of_event_threads,
			int concurrency_hint = io::DEFAULT_NUM_OF_CONCURRENT_EVENT_THREADS);

		error::ErrorCode handle_packet(ConnectionLevelDescriptor, Packet*) override;

		error::ErrorCode handle_handshake_packet(ConnectionLevelDescriptor, PacketHandshake&);

		void serve_forever();

		/**
		 *  Event handler interface
		 */

		virtual void handle_deferred_packet(DeferredPacketEvent<PacketHandshake>*) override;

	private:
		void flush_deferred_packet();

		template <typename PacketType>
		void flush_deferred_packet_internal(DeferredPacketEvent<PacketType>& event)
		{
			if (not deferred_packet_stack.is_empty<PacketType>()) {
				if (auto& status = event.status(); status == IDeferredPacketEvent::Unused) {
					status = IDeferredPacketEvent::Processing;
					event.head = deferred_packet_stack.pop<PacketType>();
					server_core.post_event(&event, this);
				}
			}
		}

		net::ServerCore server_core;
		database::DatabaseCore database_core;

		net::DeferredPacketStack deferred_packet_stack;
		DeferredPacketEvent<PacketHandshake> deferred_handshake_packet_event;

		WorkerLevelDescriptor worker_permission = WorkerLevelDescriptor(0);
	};
}