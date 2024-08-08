#pragma once

#include <net/udp_server.h>

namespace router
{
    namespace net
    {
        class RouteServer : public ::net::MessageHandler
        {
        public:
            using handler_type = bool (*)(const ::net::MessageRequest&, ::net::MessageResponse&);

            RouteServer();

            void serve_forever();

            bool handle_message(const ::net::MessageRequest&, ::net::MessageResponse&) override;

            static bool handle_get_config(const ::net::MessageRequest&, ::net::MessageResponse&);

        private:
            ::net::UdpServer server_core;
        };
    }
}