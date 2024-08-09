#pragma once

#include "net/udp_server.h"
#include "proto/generated/protocol.pb.h"

namespace net
{
    
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

        void register_server(protocol::ServerType, const net::ServerInfo&);

        bool announce_server(protocol::ServerType, const net::ServerInfo&);

        static bool read_message(net::Socket&, net::MessageRequest&, struct sockaddr_in& sender_addr, int& sender_addr_size);

        static bool read_message(net::Socket&, net::MessageRequest&);

        static bool get_config(const char* router_ip, int router_port, protocol::ServerType target, net::MessageResponse&);

    private:
        net::UdpServer& _source;

        net::ServerInfo _servers[protocol::ServerType::SIZE];
    };
}