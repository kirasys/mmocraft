#pragma once

#include <list>
#include <vector>
#include <string>
#include <memory>

#include "config/config.h"
#include "net/socket.h"
#include "net/packet.h"
#include "net/connection.h"
#include "net/server_core.h"
#include "io/io_event_pool.h"
#include "io/io_service.h"
#include "win/object_pool.h"
#include "util/common_util.h"

namespace net
{
    class ConnectionEnvironment;

    class PacketHandler
    {
    public:
        virtual error::ResultCode handle_packet(net::Connection&, const std::byte*) = 0;

        virtual void on_disconnect(net::Connection&) = 0;
    };

    class PacketHandleServerStub : public PacketHandler
    {
        error::ResultCode handle_packet(net::Connection&, const std::byte*) override
        {
            return error::code::success;
        }

        void on_disconnect(net::Connection&) override
        {
            return;
        }
    };

    class TcpServer final : public net::ServerCore, public io::IoEventHandler
    {
    public:
        TcpServer(net::PacketHandler&, net::ConnectionEnvironment&, io::RegisteredIO&);

        void start_network_io_service(std::string_view ip, int port, std::size_t num_of_event_threads) override;

        net::ConnectionKey new_connection(win::UniqueSocket &&client_sock = win::UniqueSocket());

        void start_accept();

        /**
         *  Event handler interface 
         */

        virtual void on_error() override;

        virtual void on_complete(io::IoAcceptEvent*) override;

        virtual std::size_t handle_io_event(io::IoAcceptEvent*) override;
        
    private:
        net::PacketHandler& packet_handle_server;

        ConnectionEnvironment& connection_env;
        util::IntervalTaskScheduler<ConnectionEnvironment> connection_env_task;

        net::Socket _listen_sock;

        io::RegisteredIO& io_service;

        io::IoAcceptEvent io_accept_event;
    };
}