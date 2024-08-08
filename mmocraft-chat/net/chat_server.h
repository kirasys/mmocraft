#pragma once

#include <cstddef>

#include <net/udp_server.h>
#include <net/server_communicator.h>
#include <config/config.h>

namespace chat
{
    class ChatServer : public net::MessageHandler
    {
    public:
        ChatServer();

        bool handle_message(const net::MessageRequest&, net::MessageResponse& response) override;

        void serve_forever();

    private:
        net::UdpServer server_core;
        net::ServerCommunicator _communicator;
    };
}