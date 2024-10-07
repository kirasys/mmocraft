#pragma once

#include <initializer_list>
#include <shared_mutex>
#include <optional>

#include "net/connection_key.h"
#include "net/socket.h"
#include "net/udp_message.h"
#include "proto/generated/protocol.pb.h"

namespace net
{
    class ServerCommunicator
    {
    public:
        using common_handler_type = bool (ServerCommunicator::*)(net::MessageRequest&);

        ServerCommunicator(net::Socket& src)
            : _source{ src }
        { }

        std::optional<bool> handle_common_message(::net::MessageRequest&);

        net::IPAddress get_server(protocol::server_type_id);

        bool handle_server_announcement(::net::MessageRequest&);

        void register_server(protocol::server_type_id, const net::IPAddress&);

        bool announce_server(protocol::server_type_id, const net::IPAddress&);

        bool fetch_server_address_async(protocol::server_type_id);

        bool fetch_server_address(protocol::server_type_id);

        static auto fetch_config(const char* router_ip, int router_port, protocol::server_type_id)
            -> std::pair<bool, net::MessageRequest>;

        bool forward_packet(protocol::server_type_id, net::message_id::value, net::ConnectionKey, util::byte_view);

        template <typename ConfigType>
        static bool load_remote_config(const char* router_ip, int router_port, protocol::server_type_id server_type, ConfigType& config)
        {
            auto [success, response] = net::ServerCommunicator::fetch_config(router_ip, router_port, server_type);
            if (not success) {
                std::cerr << "Fail to get config from remote(" << server_type << ')';
                return false;
            }

            protocol::FetchConfigResponse fetch_config_msg;
            if (not response.parse_message(fetch_config_msg)) {
                std::cerr << "Fail to parse GetConfigResponse";
                return false;
            }

            if (not config.ParseFromString(fetch_config_msg.config())) {
                std::cerr << "Fail to parse ChatConfig";
                return false;
            }

            return true;
        }

        template <typename ConfigType>
        bool load_remote_config(protocol::server_type_id server_type, ConfigType& config)
        {
            auto [router_ip, router_port] = get_server(protocol::server_type_id::router);
            return load_remote_config(router_ip.c_str(), router_port, server_type, config);
        }

        bool send_to(net::MessageRequest& request, protocol::server_type_id server_type)
        {
            request.set_requester(_source.get_handle());
            request.set_request_address(get_server(server_type));

            return request.flush_send();
        }

        static auto send_message_reliably(const net::MessageRequest&, int retry_count = std::numeric_limits<int>::max())
            -> std::pair<bool, net::MessageRequest>;

    private:
        net::Socket& _source;

        std::shared_mutex server_table_mutex;
        net::IPAddress _servers[protocol::server_type_id::total_count];
    };
}