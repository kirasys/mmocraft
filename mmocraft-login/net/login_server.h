#pragma once

#include <cstddef>

#include <net/connection_key.h>
#include <net/udp_server.h>
#include <net/server_communicator.h>

namespace login
{
    namespace net
    {
        class LoginServer
        {
        public:
            using handler_type = ::net::UdpServer<LoginServer>::handler_type;

            LoginServer();

            bool handle_handshake_packet(const ::net::MessageRequest&, ::net::MessageResponse&);

            bool initialize(const char* router_ip, int port);

            void serve_forever(int argc, char* argv[]);

        private:
            ::net::UdpServer<LoginServer> server_core;

            std::map<std::string, ::net::ConnectionKey, std::less<>> player_lookup_table;
        };
    }
}