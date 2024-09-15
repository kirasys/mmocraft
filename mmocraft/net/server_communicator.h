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

        bool handle_server_announcement(const ::net::MessageRequest&, ::net::MessageResponse&);

        bool fetch_server_async(protocol::ServerType);

        bool fetch_server(protocol::ServerType);

        bool fetch_config(protocol::ServerType target, net::MessageResponse&);

        static bool fetch_config(const char* router_ip, int router_port, protocol::ServerType target, net::MessageResponse&);

        bool forward_packet(protocol::ServerType, net::MessageID, net::ConnectionKey, const std::byte*, std::size_t);

        template <typename ConfigType>
        static bool load_remote_config(const char* router_ip, int router_port, protocol::ServerType server_type, ConfigType& config)
        {
            net::MessageResponse response;
            if (not net::ServerCommunicator::fetch_config(router_ip, router_port, server_type, response)) {
                std::cerr << "Fail to get config from remote(" << server_type << ')';
                return false;
            }

            protocol::FetchConfigResponse fetch_config_res;
            if (not fetch_config_res.ParseFromArray(response.begin_message(), int(response.message_size()))) {
                std::cerr << "Fail to parse GetConfigResponse";
                return false;
            }

            if (not config.ParseFromString(fetch_config_res.config())) {
                std::cerr << "Fail to parse ChatConfig";
                return false;
            }

            return true;
        }

        template <typename ConfigType>
        bool load_remote_config(protocol::ServerType server_type, ConfigType& config)
        {
            auto [router_ip, router_port] = get_server(protocol::ServerType::Router);
            return load_remote_config(router_ip.c_str(), router_port, server_type, config);
        }

        template <typename MessageType>
        bool send_message(protocol::ServerType server_type, net::MessageID msg_id, MessageType& msg)
        {
            net::MessageRequest request(msg_id);
            request.set_message(msg);

            auto [ip, port] = get_server(server_type);
            return port ? _source.send_to(ip.c_str(), port, request.cbegin(), request.size()) : false;
        }

        static bool read_message(net::Socket&, net::MessageRequest&, struct sockaddr_in& sender_addr, int& sender_addr_size);

        static bool read_message(net::Socket&, net::MessageRequest&);

        static bool send_message_reliably(const std::string& ip, int port, const net::MessageRequest&, net::MessageResponse&, int retry_count = std::numeric_limits<int>::max());

    private:
        net::Socket& _source;

        std::shared_mutex server_table_mutex;
        net::ServerInfo _servers[protocol::ServerType::SIZE];
    };
}