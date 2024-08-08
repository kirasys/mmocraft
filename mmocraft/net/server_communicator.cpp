#include "pch.h"
#include "server_communicator.h"

#include "net/udp_message.h"
#include "net/server_communicator.h"
#include "logging/logger.h"

namespace net
{
    void ServerCommunicator::register_server(protocol::ServerType server_type, const net::ServerInfo& server_info)
    {
        _servers[server_type] = server_info;
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

    bool ServerCommunicator::get_config(const char* router_ip, int router_port, protocol::ServerType target, net::MessageResponse& response)
    {
        protocol::GetConfigRequest get_config_msg;
        get_config_msg.set_server_type(target);

        net::MessageRequest request(net::MessageID::Router_GetConfig);
        request.set_message(get_config_msg);

        // Send the get config message to the router.
        net::Socket sock{ net::SocketProtocol::UDPv4 };
        sock.set_socket_option(SO_RCVTIMEO, 1000);

        CONSOLE_LOG(info) << "Trying to get config...";

        for (int i = 0; i < 3; i++) {
            sock.send_to(router_ip, router_port, request.begin(), request.size());

            if (net::ServerCommunicator::read_message(sock, response)) {
                CONSOLE_LOG(info) << "Done.";
                return true;
            }
        }

        CONSOLE_LOG(error) << "Failed to get config";
        return false;
    }
}