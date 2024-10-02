#pragma once

#include <net/udp_server.h>
#include <net/server_communicator.h>

namespace router
{
    namespace net
    {
        class RouteServer
        {
        public:
            using handler_type = ::net::UdpServer<RouteServer>::handler_type;

            RouteServer();

            void serve_forever();

            bool handle_fetch_config(::net::MessageRequest&);

            bool handle_fetch_server(::net::MessageRequest&);

        private:
            ::net::UdpServer<RouteServer> server_core;
        };
    }
}