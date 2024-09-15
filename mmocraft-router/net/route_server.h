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

            bool handle_fetch_config(const ::net::MessageRequest&, ::net::MessageResponse&);

            bool handle_fetch_server(const ::net::MessageRequest&, ::net::MessageResponse&);

        private:
            ::net::UdpServer<RouteServer> server_core;
        };
    }
}