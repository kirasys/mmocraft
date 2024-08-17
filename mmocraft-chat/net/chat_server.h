#pragma once

#include <cstddef>

#include <net/udp_server.h>
#include <net/server_communicator.h>
#include <config/config.h>

namespace chat
{
    namespace net
    {
        class ChatServer : public ::net::MessageHandler
        {
        public:
            using handler_type = bool (ChatServer::*)(const ::net::MessageRequest&, ::net::MessageResponse&);

            using packet_handler_type = bool (ChatServer::*)(const ::net::PacketRequest&, ::net::PacketResponse&);

            ChatServer();

            bool handle_message(const ::net::MessageRequest&, ::net::MessageResponse&) override;
            
            bool handle_packet(const ::net::MessageRequest&, ::net::MessageResponse&);

            void serve_forever(int argc, char* argv[]);

        private:
            ::net::UdpServer server_core;
            ::net::ServerCommunicator _communicator;
        };
    }
}