#pragma once

#include <net/udp_server.h>
#include <net/server_communicator.h>

namespace router
{
    namespace net
    {
        class RouteServer : ::net::MessageHandler
        {
        public:

            RouteServer();

            void serve_forever();

            virtual bool handle_message(::net::MessageRequest&) override;

            bool handle_fetch_config(::net::MessageRequest&);

            bool handle_fetch_server(::net::MessageRequest&);

        private:
            ::net::UdpServer server_core;
        };
    }
}