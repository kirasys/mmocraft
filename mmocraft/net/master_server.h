#pragma once

#include "database/database_core.h"

#include "net/connection_server.h"
#include "net/deferred_packet.h"
#include "net/server_core.h"
#include "net/packet.h"
#include "util/interval_task.h"

namespace net
{
    class MasterServer : public net::PacketHandleServer
    {
    public:
        MasterServer();

        error::ResultCode handle_packet(net::ConnectionServer::Descriptor&, Packet*) override;

        error::ResultCode handle_handshake_packet(net::ConnectionServer::Descriptor&, PacketHandshake&);

        void serve_forever();

        /**
         *  Deferred packet handler methods.
         */

        virtual void handle_deferred_handshake_packet(net::PacketEvent*, const DeferredPacket<PacketHandshake>*);

    private:

        void process_deferred_packet_result();

        void process_deferred_packet_result_internal(const DeferredPacketResult*);

        void flush_deferred_packet();

        util::IntervalTaskScheduler<void> interval_task_scheduler;

        net::ServerCore server_core;

        database::DatabaseCore database_core;

        DeferredPacketEvent<PacketHandshake, MasterServer> deferred_handshake_packet_event;
        PacketEvent* deferred_packet_events[1] = {
            &deferred_handshake_packet_event
        };
    };
}