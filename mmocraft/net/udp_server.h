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

    template <typename ServerType>
    class UdpServer : public net::ServerCore
    {
    public:

        using handler_type = bool (ServerType::*)(const net::MessageRequest&, net::MessageResponse&);

        UdpServer(ServerType* server_inst, std::array<handler_type, 0x100>* msg_handler_table)
            : listen_sock{ net::SocketProtocol::UDPv4 }
            , app_server{ server_inst }
            , message_handler_table{ msg_handler_table }
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
            net::MessageResponse response;

            struct sockaddr_in sender_addr;
            int sender_addr_size = sizeof(sender_addr);

            while (not is_terminated) {
                if (not net::ServerCommunicator::read_message(listen_sock, request, sender_addr, sender_addr_size))
                    continue;

                response.set_message_id(request.message_id());

                // handle message and send reply.
                if (handle_message(request, response) && response.message_size()) {
                    auto transferred_bytes = ::sendto(listen_sock.get_handle(),
                        response.begin(), int(response.size()),
                        0,
                        (SOCKADDR*)&sender_addr, sender_addr_size);

                    LOG_IF(error, transferred_bytes == SOCKET_ERROR)
                        << "sendto() failed with " << ::WSAGetLastError();
                    LOG_IF(error, transferred_bytes != SOCKET_ERROR && transferred_bytes < response.size())
                        << "sendto() successed partially";

                    response.reset();
                }
            }
        }

        bool handle_message(const MessageRequest& request, MessageResponse& response)
        {
            if (auto handler = (*message_handler_table)[request.message_id()])
                return (app_server->*handler)(request, response);

            CONSOLE_LOG(error) << "Unimplemented message id : " << request.message_id();
            return false;
        }

    private:
        net::Socket listen_sock;

        bool is_terminated = false;
        std::vector<std::thread> event_threads;

        ServerType* const app_server = nullptr;
        std::array<handler_type, 0x100>* const message_handler_table = nullptr;

        net::ServerCommunicator _communicator;
    };
}