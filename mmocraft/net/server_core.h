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
#include "io/io_event_pool.h"
#include "io/io_service.h"
#include "win/object_pool.h"
#include "util/common_util.h"

namespace net
{
    class ConnectionEnvironment;

    class ServerCore final : public io::IoEventHandler
    {
    public:
        enum State
        {
            Uninitialized,
            Initialized,
            Running,
            Stopped,
        };

        ServerCore(net::PacketHandleServer&, net::ConnectionEnvironment&, io::IoService&);

        ServerCore::State status() const
        {
            return _state;
        }

        error::ResultCode get_last_error() const
        {
            return last_error_code;
        }

        void start_network_io_service();

        net::ConnectionKey new_connection(win::UniqueSocket &&client_sock = win::UniqueSocket());

        /**
         *  Event handler interface 
         */

        virtual void on_error() override;

        virtual void on_complete(io::IoAcceptEvent*) override;

        virtual std::size_t handle_io_event(io::IoAcceptEvent*) override;
        
    private:
        ServerCore::State _state = Uninitialized;
        error::ResultCode last_error_code;

        PacketHandleServer& packet_handle_server;

        ConnectionEnvironment& connection_env;
        util::IntervalTaskScheduler<ConnectionEnvironment> connection_env_task;

        net::Socket _listen_sock;

        io::IoService& io_service;

        io::IoAcceptEventData io_accept_event_data;
        io::IoAcceptEvent io_accept_event{ &io_accept_event_data };
    };
}