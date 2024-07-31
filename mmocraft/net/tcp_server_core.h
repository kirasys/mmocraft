#pragma once

#include <list>
#include <vector>
#include <string>
#include <memory>

#include "config/config.h"
#include "net/socket.h"
#include "net/packet.h"
#include "net/connection.h"
#include "net/deferred_packet.h"
#include "net/server_core.h"
#include "io/io_event_pool.h"
#include "io/io_service.h"
#include "win/object_pool.h"
#include "util/common_util.h"

namespace net
{
    class ConnectionEnvironment;

    class TcpServerCore final : public net::ServerCore, public io::IoEventHandler
    {
    public:
        TcpServerCore(net::PacketHandleServer&, net::ConnectionEnvironment&, io::IoService&);

        void start_network_io_service() override;

        net::ConnectionKey new_connection(win::UniqueSocket &&client_sock = win::UniqueSocket());

        void start_accept();

        /**
         *  Event handler interface 
         */

        virtual void on_error() override;

        virtual void on_complete(io::IoAcceptEvent*) override;

        virtual std::size_t handle_io_event(io::IoAcceptEvent*) override;
        
    private:
        PacketHandleServer& packet_handle_server;

        ConnectionEnvironment& connection_env;
        util::IntervalTaskScheduler<ConnectionEnvironment> connection_env_task;

        net::Socket _listen_sock;

        io::IoService& io_service;

        io::IoAcceptEventData io_accept_event_data;
        io::IoAcceptEvent io_accept_event{ &io_accept_event_data };
    };
}