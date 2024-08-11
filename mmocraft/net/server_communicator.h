#pragma once

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

        const net::ServerInfo& get_server(protocol::ServerType);

        void register_server(protocol::ServerType, const net::ServerInfo&);

        bool announce_server(protocol::ServerType, const net::ServerInfo&);

        //bool (protocol::ServerType);

        static bool read_message(net::Socket&, net::MessageRequest&, struct sockaddr_in& sender_addr, int& sender_addr_size);

        static bool read_message(net::Socket&, net::MessageRequest&);

        static bool send_message_reliably(const char* ip, int port, const net::MessageRequest&, net::MessageResponse&, int retry_count = std::numeric_limits<int>::max());

        static bool get_config(const char* router_ip, int router_port, protocol::ServerType target, net::MessageResponse&);

    private:
        net::UdpServer& _source;

        net::ServerInfo _servers[protocol::ServerType::SIZE];
    };
}