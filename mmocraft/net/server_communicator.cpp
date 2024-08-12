#include "pch.h"
#include "server_communicator.h"

#include "net/udp_message.h"
#include "net/server_communicator.h"
#include "logging/logger.h"

namespace net
{
    const net::ServerInfo& ServerCommunicator::get_server(protocol::ServerType server_type)
    {
        return _servers[server_type];
    }

    void ServerCommunicator::register_server(protocol::ServerType server_type, const net::ServerInfo& server_info)
    {
        _servers[server_type] = server_info;
    }

    bool ServerCommunicator::announce_server(protocol::ServerType server_type, const net::ServerInfo& server_info)
    {
        protocol::ServerAnnouncement announce_msg;
        announce_msg.set_server_type(server_type);
        announce_msg.mutable_server_info()->set_ip(server_info.ip);
        announce_msg.mutable_server_info()->set_port(server_info.port);

        net::MessageRequest request(net::MessageID::Router_ServerAnnouncement);
        request.set_message(announce_msg);

        auto& [router_ip, router_port] = _servers[protocol::ServerType::Router];
        return router_port ? _source.send(router_ip.c_str(), router_port, request) : false;
    }

    bool ServerCommunicator::read_message(net::Socket& sock, net::MessageRequest& message, struct sockaddr_in& sender_addr, int& sender_addr_size)
    {
        auto transferred_bytes = ::recvfrom(
            sock.get_handle(),
            message.begin(), int(message.capacity()),
            0,
            (SOCKADDR*)&sender_addr, &sender_addr_size
        );

        if (transferred_bytes == SOCKET_ERROR || transferred_bytes == 0) {
            auto errorcode = ::WSAGetLastError();
            LOG_IF(error, errorcode != 10004 && errorcode != 10038)
                << "recvfrom() failed with :" << errorcode;
            return false;
        }

        message.set_size(transferred_bytes);
        return true;
    }
        
    bool ServerCommunicator::read_message(net::Socket& sock, net::MessageRequest& message)
    {
        struct sockaddr_in sender_addr;
        int sender_addr_size = sizeof(sender_addr);
        return read_message(sock, message, sender_addr, sender_addr_size);
    }

    bool ServerCommunicator::send_message_reliably(const char* ip, int port, const net::MessageRequest& request, net::MessageResponse& response, int retry_count)
    {
        net::Socket sock{ net::SocketProtocol::UDPv4 };
        sock.set_socket_option(SO_RCVTIMEO, UDP_MESSAGE_RETRANSMISSION_PERIOD);
        
        for (int i = retry_count; i >= 0 ; i--) {
            auto sended_tick = util::current_monotonic_tick();
            sock.send_to(ip, port, request.cbegin(), request.size());

            if (net::ServerCommunicator::read_message(sock, response))
                return true;

            if (i > 0 && util::current_monotonic_tick() - sended_tick < UDP_MESSAGE_RETRANSMISSION_PERIOD)
                util::sleep_ms(UDP_MESSAGE_RETRANSMISSION_PERIOD);
        }

        return false;
    }

    bool ServerCommunicator::get_config(const char* router_ip, int router_port, protocol::ServerType target, net::MessageResponse& response)
    {
        protocol::GetConfigRequest get_config_msg;
        get_config_msg.set_server_type(target);

        net::MessageRequest request(net::MessageID::Router_GetConfig);
        request.set_message(get_config_msg);

        // Send the get config message to the router.
        CONSOLE_LOG(info) << "Wait to fetch config...";
        send_message_reliably(router_ip, router_port, request, response);
        CONSOLE_LOG(info) << "Done.";

        return true;
    }
}