#pragma once

#include <list>
#include <vector>
#include <string>
#include <memory>

#include "net/socket.h"
#include "net/packet.h"
#include "net/connection.h"
#include "net/deferred_packet.h"
#include "io/io_event_pool.h"
#include "io/io_service.h"
#include "win/object_pool.h"
#include "util/common_util.h"
#include "config/config.h"

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

        ServerCore(PacketHandleServer&, ConnectionEnvironment&, const config::Configuration_Server& conf = config::get_server_config());

        ServerCore::State status() const
        {
            return _state;
        }

        error::ResultCode get_last_error() const
        {
            return last_error_code;
        }

        void start_network_io_service();

        bool post_event(PacketEvent* event, ULONG_PTR event_handler_class);

        void new_connection(win::UniqueSocket &&client_sock);

        /**
         *  Event handler interface 
         */

        virtual void on_complete(io::IoAcceptEvent*) override;

        virtual std::size_t handle_io_event(io::IoAcceptEvent*) override;
        
    private:
        ServerCore::State _state = Uninitialized;
        error::ResultCode last_error_code;

        PacketHandleServer& packet_handle_server;

        ConnectionEnvironment& connection_env;
        util::IntervalTaskScheduler<ConnectionEnvironment> connection_env_task;

        net::Socket _listen_sock;

        io::IoCompletionPort io_service;

        io::IoEventPool io_event_pool;
        win::ObjectPool<io::IoAcceptEventData>::Pointer io_accept_event_data;
        win::ObjectPool<io::IoAcceptEvent>::Pointer io_accept_event;

        win::ObjectPool<net::Connection> connection_pool;
    };
}