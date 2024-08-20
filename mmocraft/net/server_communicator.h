#pragma once

#include <initializer_list>
#include <shared_mutex>

#include "net/connection_key.h"
#include "net/socket.h"
#include "net/udp_message.h"
#include "proto/generated/protocol.pb.h"

namespace net
{
    constexpr int UDP_MESSAGE_RETRANSMISSION_PERIOD = 3000;

    struct ServerInfo
    {
        std::string ip = "";
        int port = 0;
    };

    class ServerCommunicator
    {
    public:
        ServerCommunicator(net::Socket& src)
            : _source{ src }
        { }

        net::ServerInfo get_server(protocol::ServerType);

        void register_server(protocol::ServerType, const net::ServerInfo&);

        bool announce_server(protocol::ServerType, const net::ServerInfo&);

        bool fetch_server_async(protocol::ServerType);

        bool fetch_server(protocol::ServerType);

        bool forward_packet(protocol::ServerType, net::ConnectionKey, const std::byte*, std::size_t);

        template <typename MessageType>
        bool send_message_to_router(MessageType msg, net::MessageID msg_id)
        {
            net::MessageRequest request(msg_id);
            request.set_message(msg);

            auto [router_ip, router_port] = get_server(protocol::ServerType::Router);
            return router_port ? _source.send_to(router_ip.c_str(), router_port, request.cbegin(), request.size()) : false;
        }

        static bool read_message(net::Socket&, net::MessageRequest&, struct sockaddr_in& sender_addr, int& sender_addr_size);

        static bool read_message(net::Socket&, net::MessageRequest&);

        static bool send_message_reliably(const char* ip, int port, const net::MessageRequest&, net::MessageResponse&, int retry_count = std::numeric_limits<int>::max());

        static bool fetch_config(const char* router_ip, int router_port, protocol::ServerType target, net::MessageResponse&);

    private:
        net::Socket& _source;

        std::shared_mutex server_table_mutex;
        net::ServerInfo _servers[protocol::ServerType::SIZE];
    };
}