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
		MasterServer();

		error::ResultCode handle_packet(DescriptorType::Connection, Packet*) override;

		error::ResultCode handle_handshake_packet(DescriptorType::Connection, PacketHandshake&);

		void serve_forever();

		/**
		 *  Deferred packet handler methods.
		 */

		virtual void handle_deferred_packet(DeferredPacketEvent<PacketHandshake>*, const DeferredPacket<PacketHandshake>*) override;

	private:
		void process_deferred_packet_result();

		void process_deferred_packet_result_internal(const DeferredPacketResult*);

		void flush_deferred_packet();

		net::ServerCore server_core;

		database::DatabaseCore database_core;

		DeferredPacketEvent<PacketHandshake> deferred_handshake_packet_event;
		PacketEvent* deferred_packet_events[1] = {
			&deferred_handshake_packet_event
		};
	};
}