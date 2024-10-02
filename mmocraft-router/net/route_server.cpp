#include "route_server.h"

#include <array>
#include <proto/generated/protocol.pb.h>
#include <logging/logger.h>
#include <util/time_util.h>

#include "../config/config.h"

namespace
{
    std::array<bool (router::net::RouteServer::*)(net::MessageRequest&), 0x100> message_handler_table = [] {
        std::array<bool (router::net::RouteServer::*)(net::MessageRequest&), 0x100> arr{};
        arr[::net::message_id::fetch_config] = &router::net::RouteServer::handle_fetch_config;
        arr[::net::message_id::fetch_server_address] = &router::net::RouteServer::handle_fetch_server_address;
        return arr;
    }();

}

namespace router {
namespace net {
    RouteServer::RouteServer()
        : server_core{ *this }
    {
        
    }

    void RouteServer::serve_forever()
    {
        auto& server_conf = config::get_server_config();
        server_core.start_network_io_service(server_conf.ip(), server_conf.port(), 1);

        server_core.communicator().register_server(protocol::server_type_id::router, {server_conf.ip(), server_conf.port()});

        while (true) {
            util::sleep_ms(3000);
        }
    }

    bool RouteServer::handle_message(::net::MessageRequest& request)
    {
        if (auto handler = message_handler_table[request.message_id()])
            return (this->*handler)(request);

        return false;
    }

    bool RouteServer::handle_fetch_config(::net::MessageRequest& request)
    {
        protocol::FetchConfigRequest msg;
        if (not request.parse_message(msg))
            return false;

        protocol::FetchConfigResponse config_msg;
        if (not router::config::load_server_config(msg.server_type(), &config_msg))
            return false;

        request.send_reply(config_msg);
        return true;
    }

    bool RouteServer::handle_fetch_server_address(::net::MessageRequest& request)
    {
        protocol::FetchServerRequest msg;
        if (not request.parse_message(msg))
            return false;

        auto requested_server_info = server_core.communicator().get_server(msg.server_type());
        if (not requested_server_info.port)
            return false;

        protocol::FetchServerResponse result_msg;
        result_msg.set_server_type(msg.server_type());
        result_msg.mutable_server_info()->set_ip(requested_server_info.ip);
        result_msg.mutable_server_info()->set_port(requested_server_info.port);
        
        request.send_reply(result_msg);
        return true;
    }
}
}