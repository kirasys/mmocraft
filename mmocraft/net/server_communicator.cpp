#include "pch.h"
#include "server_communicator.h"

#include <array>

#include "config/constants.h"

#include "net/udp_message.h"
#include "proto/generated/protocol.pb.h"
#include "logging/logger.h"

namespace
{
    std::array<net::ServerCommunicator::common_handler_type, 0x100> common_message_handler_table = [] {
        std::array<net::ServerCommunicator::common_handler_type, 0x100> arr{};
        arr[net::message_id::server_announcement] = &net::ServerCommunicator::handle_server_announcement;
        return arr;
    }();
}

namespace net
{
    std::optional<bool> ServerCommunicator::handle_common_message(::net::MessageRequest& request)
    {
        if (auto handler = common_message_handler_table[request.message_id()])
            return (this->*handler)(request);
        else
            return std::nullopt;
    }

    net::IPAddress ServerCommunicator::get_server(protocol::server_type_id server_type)
    {
        std::shared_lock lock(server_table_mutex);
        return _servers[server_type];
    }

    void ServerCommunicator::register_server(protocol::server_type_id server_type, const net::IPAddress& server_info)
    {
        std::unique_lock lock(server_table_mutex);
        _servers[server_type] = server_info;
    }

    bool ServerCommunicator::announce_server(protocol::server_type_id target_server_type, const net::IPAddress& target_server_addr)
    {
        protocol::ServerAnnouncement announce_msg;
        announce_msg.set_server_type(target_server_type);
        announce_msg.mutable_server_info()->set_ip(target_server_addr.ip);
        announce_msg.mutable_server_info()->set_port(target_server_addr.port);

        // Send the announcement message to the router.
        net::MessageRequest req(net::message_id::server_announcement);
        return send_to(req, protocol::server_type_id::router, announce_msg);
    }

    bool ServerCommunicator::handle_server_announcement(::net::MessageRequest& request)
    {
        protocol::ServerAnnouncement msg;
        if (not request.parse_message(msg))
            return false;

        register_server(msg.server_type(), {
            .ip = msg.server_info().ip(),
            .port = msg.server_info().port()
            });

        return true;
    }

    bool ServerCommunicator::fetch_server_address(protocol::server_type_id server_type)
    {
        protocol::FetchServerRequest fetch_server_msg;
        fetch_server_msg.set_server_type(server_type);

        // Send the get config message to the router.
        net::MessageRequest request(net::message_id::fetch_server_address);
        request.set_message(fetch_server_msg);
        request.set_request_address(get_server(protocol::server_type_id::router));

        CONSOLE_LOG(info) << "Wait to fetch server("<< server_type << ") info...";
        auto [_, response] = send_message_reliably(request);
        CONSOLE_LOG(info) << "Done.";

        // Register fetched server.
        protocol::FetchServerResponse fetch_server_res;
        if (not response.parse_message(fetch_server_res)) {
            CONSOLE_LOG(error) << "Fail to parse FetchServerResponse";
            return false;
        }

        register_server(server_type, { fetch_server_res.server_info().ip(), fetch_server_res.server_info().port() });
        return true;
    }

    bool ServerCommunicator::fetch_server_address_async(protocol::server_type_id server_type)
    {
        protocol::FetchServerRequest fetch_server_msg;
        fetch_server_msg.set_server_type(server_type);

        net::MessageRequest request(net::message_id::fetch_server_address);
        return send_to(request, protocol::server_type_id::router, fetch_server_msg);
    }

    auto ServerCommunicator::fetch_config(const char* router_ip, int router_port, protocol::server_type_id target)
        -> std::pair<bool, net::MessageRequest>
    {
        protocol::FetchConfigRequest fetch_config_msg;
        fetch_config_msg.set_server_type(target);

        net::MessageRequest request(net::message_id::fetch_config);
        request.set_message(fetch_config_msg);
        request.set_request_address({ router_ip , router_port});

        // Send the get config message to the router.
        CONSOLE_LOG(info) << "Wait to fetch config...";
        auto&& res = send_message_reliably(request);
        CONSOLE_LOG(info) << "Done.";

        return res;
    }

    bool ServerCommunicator::forward_packet(protocol::server_type_id server_type, net::message_id::value packet_type, net::ConnectionKey source, const std::byte* data, std::size_t data_size)
    {
        protocol::PacketHandleRequest packet_handle_msg;
        packet_handle_msg.set_connection_key(source.raw());
        packet_handle_msg.set_packet_data(std::string{ reinterpret_cast<const char*>(data), data_size});

        net::MessageRequest request(packet_type);
        return send_to(request, server_type, packet_handle_msg);
    }

    auto ServerCommunicator::send_message_reliably(const net::MessageRequest& orig_request, int retry_count)
        -> std::pair<bool, net::MessageRequest>
    {
        // set ephemeral requester.
        net::Socket ephemeral_sock{ net::socket_protocol_id::udp_v4 };
        ephemeral_sock.set_socket_option(SO_RCVTIMEO, config::network::udp_message_retransmission_period);

        net::MessageRequest request(orig_request);
        request.set_requester(ephemeral_sock.get_handle());
        net::MessageRequest response(request);

        for (int i = retry_count; i >= 0 ; i--) {
            auto sended_tick = util::current_monotonic_tick();

            // wait a reply.
            if (request.flush_send() && response.read_message())
                return { true, response };

            if (i > 0 && util::current_monotonic_tick() - sended_tick < config::network::udp_message_retransmission_period)
                util::sleep_ms(config::network::udp_message_retransmission_period);
        }

        return { false, {} };
    }
}