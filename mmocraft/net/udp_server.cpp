#include "pch.h"
#include "udp_server.h"

#include "net/server_communicator.h"
#include "config/config.h"
#include "logging/logger.h"

namespace net
{
    UdpServer::UdpServer(MessageHandler& msg_handler)
        : _sock{ net::SocketProtocol::UDPv4 }
        , message_handler{ msg_handler }
    {
        if (not _sock.set_socket_option(SO_RCVBUF, SOCKET_RCV_BUFFER_SIZE) ||
            not _sock.set_socket_option(SO_SNDBUF, SOCKET_SND_BUFFER_SIZE))
            throw error::SOCKET_SETOPT;
    }

    UdpServer::~UdpServer()
    {
        reset();
    }

    void UdpServer::reset()
    {
        _sock.reset(net::SocketProtocol::UDPv4);
        is_terminated = true;

        for (auto& event_thread : event_threads)
            event_thread.join();

        event_threads.clear();
    }

    bool UdpServer::send(const std::string& ip, int port, const net::MessageRequest& msg)
    {
        return _sock.send_to(ip.c_str(), port, msg.cbegin(), msg.size());
    }

    void UdpServer::start_network_io_service(std::string_view ip, int port, std::size_t num_of_event_threads)
    {
        if (not _sock.bind(ip, port))
            return;

        CONSOLE_LOG(info) << "Listening to " << ip << ':' << port << "...";

        for (unsigned i = 0; i < num_of_event_threads; i++)
            event_threads.emplace_back(spawn_event_loop_thread());

    }

    std::thread UdpServer::spawn_event_loop_thread()
    {
        return std::thread([](UdpServer* udp_server) {
            udp_server->run_event_loop_forever(0);
        }, this);
    }

    void UdpServer::run_event_loop_forever(DWORD)
    {
        MessageRequest request;
        MessageResponse response;

        struct sockaddr_in sender_addr;
        int sender_addr_size = sizeof(sender_addr);

        while (not is_terminated) {
            if (not net::ServerCommunicator::read_message(_sock, request, sender_addr, sender_addr_size))
                continue;

            response.set_message_id(request.message_id());

            if (message_handler.handle_message(request, response) && response.message_size()) {
                auto transferred_bytes = ::sendto(_sock.get_handle(),
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
}