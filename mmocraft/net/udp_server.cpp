#include "pch.h"
#include "udp_server.h"

#include "config/config.h"
#include "logging/logger.h"

namespace net
{
    UdpServer::UdpServer(std::string_view ip, int port)
        : _ip{ ip }
        , _port{ port }
        , _sock{ net::SocketProtocol::UDPv4 }
    {
        if (not _sock.bind(ip, port))
            throw error::SOCKET_BIND;

        if (not _sock.set_socket_option(SO_RCVBUF, SOCKET_RCV_BUFFER_SIZE) || 
            not _sock.set_socket_option(SO_SNDBUF, SOCKET_SND_BUFFER_SIZE))
            throw error::SOCKET_SETOPT;
    }

    UdpServer::~UdpServer()
    {
        close();
    }

    void UdpServer::close()
    {
        _sock.close();
        is_terminated = true;

        for (auto& event_thread : event_threads)
            event_thread.join();
    }

    void UdpServer::start_network_io_service()
    {
        CONSOLE_LOG(info) << "Listening to " << _ip << ':' << _port << "...\n";

        auto& system_conf = config::get_system_config();
        for (unsigned i = 0; i < system_conf.num_of_processors() * 2; i++)
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
        char buffer[1024];
        
        struct sockaddr_in sender_addr;
        int sender_addr_size = sizeof(sender_addr);

        while (not is_terminated) {
            auto transferred_bytes = ::recvfrom(_sock.get_handle(), buffer, sizeof(buffer), 0, (SOCKADDR*)&sender_addr, &sender_addr_size);
            if (transferred_bytes == SOCKET_ERROR) {
                auto errorcode = ::WSAGetLastError();
                LOG_IF(error, errorcode != 10004 && errorcode != 10038)
                    << "recvfrom() failed with :" << errorcode;
                return;
            }

            std::cout << "transferred_bytes: " << transferred_bytes << '\n';
        }
    }
}