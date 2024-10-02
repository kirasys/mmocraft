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
    constexpr int SOCKET_RCV_BUFFER_SIZE = 1024 * 1024 * 4; // 4MB
    constexpr int SOCKET_SND_BUFFER_SIZE = 1024 * 1024 * 4; // 4MB

    class MessageHandler
    {
    public:
        virtual bool handle_message(net::MessageRequest&) = 0;
    };

    class UdpServer : public net::ServerCore
    {
    public:

        UdpServer(MessageHandler& server_inst)
            : listen_sock{ net::SocketProtocol::UDPv4 }
            , app_server{ server_inst }
            , _communicator{ listen_sock }
        {
            if (not listen_sock.set_socket_option(SO_RCVBUF, SOCKET_RCV_BUFFER_SIZE) ||
                not listen_sock.set_socket_option(SO_SNDBUF, SOCKET_SND_BUFFER_SIZE))
                throw error::SOCKET_SETOPT;
        }

        ~UdpServer()
        {
            reset();
        }

        net::ServerCommunicator& communicator()
        {
            return _communicator;
        }

        void reset()
        {
            listen_sock.reset(net::SocketProtocol::UDPv4);
            is_terminated = true;

            for (auto& event_thread : event_threads)
                event_thread.join();

            event_threads.clear();
        }

        void start_network_io_service(std::string_view ip, int port, std::size_t num_of_event_threads) override
        {
            if (not listen_sock.bind(ip, port)) {
                CONSOLE_LOG(fatal) << "Couldn't bind server " << ip << ':' << port;
                return;
            }

            CONSOLE_LOG(info) << "Listening to " << ip << ':' << port << "...";

            for (unsigned i = 0; i < num_of_event_threads; i++)
                event_threads.emplace_back(spawn_event_loop_thread());

        }

        std::thread spawn_event_loop_thread()
        {
            return std::thread([](UdpServer* udp_server) {
                udp_server->run_event_loop_forever(0);
                }, this);
        }

        void run_event_loop_forever(DWORD)
        {
            net::MessageRequest request;
            request.set_requester(listen_sock.get_handle());

            while (not is_terminated) {
                if (not request.read_message())
                    continue;

                // handle message and send reply.
                handle_message(request);
            }
        }

        bool handle_message(MessageRequest& request)
        {
            // invoke common message handlers first.

            auto common_handler_success = _communicator.handle_common_message(request);

            if (not common_handler_success.value_or(true)) {
                CONSOLE_LOG(error) << "Common handler failed" << request.message_id();
                return false;
            }

            // invoke user-defined handlers if exists

            if (app_server.handle_message(request) || common_handler_success.value_or(false))
                return true;

            CONSOLE_LOG(error) << "Fail to handle message: " << request.message_id();
            return false;
        }

    private:
        net::Socket listen_sock;

        bool is_terminated = false;
        std::vector<std::thread> event_threads;

        MessageHandler& app_server;

        net::ServerCommunicator _communicator;
    };
}