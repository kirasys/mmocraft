#pragma once

#include <initializer_list>
#include <shared_mutex>

#include "net/udp_server.h"
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
        ServerCommunicator(net::UdpServer& src)
            : _source{ src }
        { }

        net::ServerInfo get_server(protocol::ServerType);

        void register_server(protocol::ServerType, const net::ServerInfo&);

        bool announce_server(protocol::ServerType, const net::ServerInfo&);

        bool fetch_server_async(protocol::ServerType);

        bool fetch_server(protocol::ServerType);

        static bool read_message(net::Socket&, net::MessageRequest&, struct sockaddr_in& sender_addr, int& sender_addr_size);

        static bool read_message(net::Socket&, net::MessageRequest&);

        static bool send_message_reliably(const char* ip, int port, const net::MessageRequest&, net::MessageResponse&, int retry_count = std::numeric_limits<int>::max());

        static bool fetch_config(const char* router_ip, int router_port, protocol::ServerType target, net::MessageResponse&);

    private:
        net::UdpServer& _source;

        std::shared_mutex server_table_mutex;
        net::ServerInfo _servers[protocol::ServerType::SIZE];
    };
}