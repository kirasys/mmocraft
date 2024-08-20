#pragma once

#include <cstddef>

#include <net/udp_server.h>
#include <net/server_communicator.h>

namespace chat
{
    namespace net
    {
        class ChatServer : public ::net::MessageHandler
        {
        public:
            using handler_type = ::net::UdpServer<ChatServer>::handler_type;

            using packet_handler_type = ::net::UdpServer<ChatServer>::packet_handler_type;

            ChatServer();

            bool handle_chat_packet(const ::net::PacketRequest&, ::net::MessageResponse&);

            bool initialize(const char* router_ip, int router_port);

            void serve_forever(int argc, char* argv[]);

        private:
            ::net::UdpServer<ChatServer> server_core;
        };
    }
}