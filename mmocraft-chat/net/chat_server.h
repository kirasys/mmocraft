#pragma once

#include <cstddef>

#include <net/udp_server.h>
#include <net/server_communicator.h>

#include <database/couchbase_core.h>

#include <util/double_buffering.h>
#include <util/interval_task.h>

namespace chat
{
    namespace net
    {
        class ChatServer : public ::net::MessageHandler
        {
        public:
            
            static constexpr protocol::server_type_id server_type = protocol::server_type_id::chat;

            ChatServer();

            virtual bool handle_message(::net::MessageRequest&) override;

            io::DetachedTask handle_chat_command(::net::MessageRequest&);

            bool initialize(const char* router_ip, int router_port);

            void serve_forever(int argc, char* argv[]);

            void announce_server();

        private:
            ::net::UdpServer server_core;

            ::util::IntervalTaskScheduler<ChatServer> interval_tasks;
        };
    }
}