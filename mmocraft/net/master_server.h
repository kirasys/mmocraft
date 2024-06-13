#pragma once

#include "database/database_core.h"

#include "net/connection.h"
#include "net/connection_environment.h"
#include "net/deferred_packet.h"
#include "net/server_core.h"
#include "net/packet.h"

namespace net
{
    class MasterServer : public net::PacketHandleServer
    {
    public:
        MasterServer(const config::Configuration_Server& conf = config::get_server_config());

        error::ResultCode handle_packet(net::Connection::Descriptor&, Packet*) override;

        error::ResultCode handle_handshake_packet(net::Connection::Descriptor&, PacketHandshake&);

        void tick();

        void serve_forever();

        /**
         *  Deferred packet handler methods.
         */

        void handle_deferred_handshake_packet_result(const DeferredPacketResult*);

        void handle_deferred_handshake_packet(net::PacketEvent*, const DeferredPacket<PacketHandshake>*);

    private:

        void handle_deferred_packet_result();

        void flush_deferred_packet();

        net::ConnectionEnvironment connection_env;

        net::ServerCore server_core;

        database::DatabaseCore database_core;

        DeferredPacketEvent<PacketHandshake, MasterServer> deferred_handshake_packet_event;
        PacketEvent* deferred_packet_events[1] = {
            &deferred_handshake_packet_event
        };

    };
}