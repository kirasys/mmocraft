#pragma once

#include "database/database_core.h"

#include "game/world.h"

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

        void handle_deferred_handshake_result(const DeferredPacketResult*);

        void handle_deferred_handshake_packet(io::Task*, const DeferredPacket<PacketHandshake>*);

    private:

        void handle_deferred_packet_result();

        void flush_deferred_packet();

        net::ConnectionEnvironment connection_env;

        net::ServerCore server_core;

        database::DatabaseCore database_core;

        game::World world;

        DeferredPacketTask<PacketHandshake, MasterServer> deferred_handshake_packet_task;
        io::Task* deferred_packet_tasks[1] = {
            &deferred_handshake_packet_task
        };

    };
}