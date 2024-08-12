#pragma once

#include <net/udp_server.h>
#include <net/server_communicator.h>

namespace router
{
    namespace net
    {
        class RouteServer : public ::net::MessageHandler
        {
        public:
            using handler_type = bool (RouteServer::*)(const ::net::MessageRequest&, ::net::MessageResponse&);

            RouteServer();

            void serve_forever();

            bool handle_message(const ::net::MessageRequest&, ::net::MessageResponse&) override;

            bool handle_get_config(const ::net::MessageRequest&, ::net::MessageResponse&);

            bool handle_fetch_server(const ::net::MessageRequest&, ::net::MessageResponse&);

            bool handle_server_announcement(const ::net::MessageRequest&, ::net::MessageResponse&);

        private:
            ::net::UdpServer server_core;
            ::net::ServerCommunicator _communicator;
        };
    }
}