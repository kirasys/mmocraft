#pragma once

#include <cstddef>

#include <database/database_core.h>
#include <database/couchbase_core.h>
#include <net/connection_key.h>
#include <net/udp_server.h>
#include <net/server_communicator.h>

#include <util/interval_task.h>

namespace login
{
    namespace net
    {
        class LoginServer
        {
        public:
            static constexpr protocol::ServerType server_type = protocol::ServerType::Login;

            using handler_type = ::net::UdpServer<LoginServer>::handler_type;

            LoginServer();

            bool handle_handshake_packet(const ::net::MessageRequest&, ::net::MessageResponse&);

            ::database::AsyncTask handle_handshake_packet(::net::MessageRequest);

            bool handle_player_logout_message(const ::net::MessageRequest&, ::net::MessageResponse&);

            bool initialize(const char* router_ip, int port);

            void serve_forever(int argc, char* argv[]);

            void announce_server();

        private:
            ::net::UdpServer<LoginServer> server_core;
            ::database::DatabaseCore player_db;
            ::database::CouchbaseCore session_db;

            ::util::IntervalTaskScheduler<LoginServer> interval_tasks;
        };
    }
}