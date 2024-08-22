#include "pch.h"
#include "server_communicator.h"

#include "net/udp_message.h"
#include "proto/generated/protocol.pb.h"
#include "logging/logger.h"

namespace net
{
    net::ServerInfo ServerCommunicator::get_server(protocol::ServerType server_type)
    {
        std::shared_lock lock(server_table_mutex);
        return _servers[server_type];
    }

    void ServerCommunicator::register_server(protocol::ServerType server_type, const net::ServerInfo& server_info)
    {
        std::unique_lock lock(server_table_mutex);
        _servers[server_type] = server_info;
    }

    bool ServerCommunicator::announce_server(protocol::ServerType server_type, const net::ServerInfo& server_info)
    {
        protocol::ServerAnnouncement announce_msg;
        announce_msg.set_server_type(server_type);
        announce_msg.mutable_server_info()->set_ip(server_info.ip);
        announce_msg.mutable_server_info()->set_port(server_info.port);

        return send_message(protocol::ServerType::Router, net::MessageID::Router_ServerAnnouncement, announce_msg);
    }

    bool ServerCommunicator::fetch_server(protocol::ServerType server_type)
    {
        protocol::FetchServerRequest fetch_server_msg;
        fetch_server_msg.set_server_type(server_type);

        net::MessageRequest request(net::MessageID::Router_FetchServer);
        request.set_message(fetch_server_msg);

        // Send the get config message to the router.
        net::MessageResponse response;
        auto [router_ip, router_port] = get_server(protocol::ServerType::Router);

        CONSOLE_LOG(info) << "Wait to fetch server("<< server_type << ") info...";
        send_message_reliably(router_ip, router_port, request, response);
        CONSOLE_LOG(info) << "Done.";

        // Register fetched server.
        protocol::FetchServerResponse fetch_server_res;
        if (not fetch_server_res.ParseFromArray(response.begin_message(), int(response.message_size()))) {
            CONSOLE_LOG(error) << "Fail to parse FetchServerResponse";
            return false;
        }

        register_server(server_type, { fetch_server_res.server_info().ip(), fetch_server_res.server_info().port() });
        return true;
    }

    bool ServerCommunicator::fetch_server_async(protocol::ServerType server_type)
    {
        protocol::FetchServerRequest fetch_server_msg;
        fetch_server_msg.set_server_type(server_type);

        return send_message(protocol::ServerType::Router, net::MessageID::Router_FetchServer, fetch_server_msg);
    }

    bool ServerCommunicator::fetch_config(protocol::ServerType target, net::MessageResponse& response)
    {
        auto [router_ip, router_port] = get_server(protocol::ServerType::Router);
        return fetch_config(router_ip.c_str(), router_port, target, response);
    }

    bool ServerCommunicator::fetch_config(const char* router_ip, int router_port, protocol::ServerType target, net::MessageResponse& response)
    {
        protocol::FetchConfigRequest fetch_config_msg;
        fetch_config_msg.set_server_type(target);

        net::MessageRequest request(net::MessageID::Router_GetConfig);
        request.set_message(fetch_config_msg);

        // Send the get config message to the router.
        CONSOLE_LOG(info) << "Wait to fetch config...";
        send_message_reliably(router_ip, router_port, request, response);
        CONSOLE_LOG(info) << "Done.";

        return true;
    }

    bool ServerCommunicator::forward_packet(protocol::ServerType server_type, net::MessageID packet_type, net::ConnectionKey source, const std::byte* data, std::size_t data_size)
    {
        protocol::PacketHandleRequest packet_handle_req;
        packet_handle_req.set_connection_key(source.raw());
        packet_handle_req.set_packet_data(std::string{ reinterpret_cast<const char*>(data), data_size});

        return send_message(server_type, packet_type, packet_handle_req);
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

    bool ServerCommunicator::send_message_reliably(const std::string& ip, int port, const net::MessageRequest& request, net::MessageResponse& response, int retry_count)
    {
        net::Socket sock{ net::SocketProtocol::UDPv4 };
        sock.set_socket_option(SO_RCVTIMEO, UDP_MESSAGE_RETRANSMISSION_PERIOD);
        
        for (int i = retry_count; i >= 0 ; i--) {
            auto sended_tick = util::current_monotonic_tick();
            sock.send_to(ip.c_str(), port, request.cbegin(), request.size());

            if (net::ServerCommunicator::read_message(sock, response))
                return true;

            if (i > 0 && util::current_monotonic_tick() - sended_tick < UDP_MESSAGE_RETRANSMISSION_PERIOD)
                util::sleep_ms(UDP_MESSAGE_RETRANSMISSION_PERIOD);
        }

        return false;
    }
}