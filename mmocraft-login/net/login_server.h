#pragma once

#include <cstddef>

#include <database/couchbase_core.h>

#include <net/connection_key.h>
#include <net/udp_server.h>
#include <net/server_communicator.h>

#include <util/interval_task.h>

namespace login
{
    namespace net
    {
        class LoginServer : public ::net::MessageHandler
        {
        public:
            static constexpr protocol::ServerType server_type = protocol::ServerType::Login;

            LoginServer();

            virtual bool handle_message(::net::MessageRequest&) override;

            database::AsyncTask handle_handshake_packet(::net::MessageRequest);

            database::AsyncTask handle_player_logout_message(::net::MessageRequest&);

            bool initialize(const char* router_ip, int port);

            void serve_forever(int argc, char* argv[]);

            void announce_server();

        private:
            ::net::UdpServer server_core;

            ::util::IntervalTaskScheduler<LoginServer> interval_tasks;
        };
    }
}