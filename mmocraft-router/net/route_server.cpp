#include "route_server.h"

#include <array>
#include <proto/generated/protocol.pb.h>
#include <logging/logger.h>
#include <util/time_util.h>

#include "../config/config.h"

namespace
{
    constinit const std::array<router::net::RouteServer::handler_type, 0xff> message_handler_db = [] {
        std::array<router::net::RouteServer::handler_type, 0xff> arr{};
        arr[::net::MessageID::Router_GetConfig] = &router::net::RouteServer::handle_get_config;
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

        while (true) {
            util::sleep_ms(3000);
        }
    }

    bool RouteServer::handle_message(const ::net::MessageRequest& request, ::net::MessageResponse& response)
    {
        if (auto handler = message_handler_db[request.message_id()])
            return handler(request, response);

        CONSOLE_LOG(error) << "Unimplemented message id : " << request.message_id();
        return false;
    }

    bool RouteServer::handle_get_config(const ::net::MessageRequest& request, ::net::MessageResponse& response)
    {
        protocol::GetConfigRequest msg;
        msg.ParseFromArray(request.begin_message(), int(request.message_size()));

        protocol::GetConfigResponse result_msg;
        if (not router::config::load_server_config(msg.server_type(), &result_msg))
            return false;

        response.set_message(result_msg);
        return true;
    }
}
}