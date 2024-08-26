#pragma once

#include <cstddef>

#include <database/database_core.h>
#include <database/mongodb_core.h>
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

            bool handle_player_logout_message(const ::net::MessageRequest&, ::net::MessageResponse&);

            bool initialize(const char* router_ip, int port);

            void serve_forever(int argc, char* argv[]);

        private:
            ::net::UdpServer<LoginServer> server_core;
            ::database::DatabaseCore player_db;
            ::database::MongoDBCore session_db;
        };
    }
}