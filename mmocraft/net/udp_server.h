#pragma once

#include <array>
#include <cassert>
#include <thread>

#include "net/socket.h"
#include "net/server_core.h"
#include "net/server_communicator.h"
#include "net/udp_message.h"
#include "logging/logger.h"

namespace net
{
    class MessageHandler
    {
    public:
        virtual bool handle_message(net::MessageRequest&) = 0;
    };

    class UdpServer : public net::ServerCore
    {
    public:

        UdpServer(MessageHandler& server_inst);

        ~UdpServer()
        {
            reset();
        }

        net::ServerCommunicator& communicator()
        {
            return _communicator;
        }

        void reset();

        void start_network_io_service(std::string_view ip, int port, std::size_t num_of_event_threads) override;

        std::thread spawn_event_loop_thread();

        void run_event_loop_forever(DWORD);

        bool handle_message(MessageRequest& request);

    private:
        net::Socket listen_sock;

        bool is_terminated = false;
        std::vector<std::thread> event_threads;

        MessageHandler& app_server;

        net::ServerCommunicator _communicator;
    };
}