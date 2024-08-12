#include "route_server.h"

#include <array>
#include <proto/generated/protocol.pb.h>
#include <logging/logger.h>
#include <util/time_util.h>

#include "../config/config.h"

namespace
{
    const std::array<router::net::RouteServer::handler_type, 0xff> message_handler_db = [] {
        std::array<router::net::RouteServer::handler_type, 0xff> arr{};
        arr[::net::MessageID::Router_GetConfig] = &router::net::RouteServer::handle_get_config;
        arr[::net::MessageID::Router_ServerAnnouncement] = &router::net::RouteServer::handle_server_announcement;
        arr[::net::MessageID::Router_FetchServer] = &router::net::RouteServer::handle_fetch_server;
        return arr;
    }();

}

namespace router {
namespace net {
    RouteServer::RouteServer()
        : server_core{ *this }
        , _communicator{ server_core }
    {
        
    }

    void RouteServer::serve_forever()
    {
        auto& server_conf = config::get_server_config();
        server_core.start_network_io_service(server_conf.ip(), server_conf.port(), 1);

        _communicator.register_server(protocol::ServerType::Router, { server_conf.ip(), server_conf.port() });

        while (true) {
            util::sleep_ms(3000);
        }
    }

    bool RouteServer::handle_message(const ::net::MessageRequest& request, ::net::MessageResponse& response)
    {
        if (auto handler = message_handler_db[request.message_id()])
            return (this->*handler)(request, response);

        CONSOLE_LOG(error) << "Unimplemented message id : " << request.message_id();
        return false;
    }

    bool RouteServer::handle_get_config(const ::net::MessageRequest& request, ::net::MessageResponse& response)
    {
        protocol::GetConfigRequest msg;
        if (not msg.ParseFromArray(request.begin_message(), int(request.message_size())))
            return false;

        protocol::GetConfigResponse result_msg;
        if (not router::config::load_server_config(msg.server_type(), &result_msg))
            return false;

        response.set_message(result_msg);
        return true;
    }

    bool RouteServer::handle_fetch_server(const ::net::MessageRequest& request, ::net::MessageResponse& response)
    {
        protocol::FetchServerRequest msg;
        if (not msg.ParseFromArray(request.begin_message(), int(request.message_size())))
            return false;

        auto requested_server_info = _communicator.get_server(msg.server_type());
        if (not requested_server_info.port)
            return false;

        protocol::FetchServerResponse result_msg;
        result_msg.set_server_type(msg.server_type());
        result_msg.mutable_server_info()->set_ip(requested_server_info.ip);
        result_msg.mutable_server_info()->set_port(requested_server_info.port);
        response.set_message(result_msg);
        
        return true;
    }

    bool RouteServer::handle_server_announcement(const ::net::MessageRequest& request, ::net::MessageResponse& response)
    {
        protocol::ServerAnnouncement msg;
        if (not msg.ParseFromArray(request.begin_message(), int(request.message_size())))
            return false;

        _communicator.register_server(msg.server_type(), {
            .ip = msg.server_info().ip(),
            .port = msg.server_info().port()
        });

        return true;
    }
}
}