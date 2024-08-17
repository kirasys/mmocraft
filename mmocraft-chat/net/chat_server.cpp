#include "chat_server.h"
#include "../config/config.h"

#include <net/packet.h>
#include <util/time_util.h>
#include <logging/logger.h>

namespace
{
    const std::array<chat::net::ChatServer::handler_type, 0xff> message_handler_db = [] {
        std::array<chat::net::ChatServer::handler_type, 0xff> arr{};
        arr[::net::MessageID::General_PacketHandle] = &chat::net::ChatServer::handle_packet;
        return arr;
    }();

    const std::array<chat::net::ChatServer::packet_handler_type, 0xff> packet_handler_db = [] {
        std::array<chat::net::ChatServer::packet_handler_type, 0xff> arr{};
        //arr[::net::Packet]
        return arr;
    }();
}

namespace chat
{
namespace net
{
    ChatServer::ChatServer()
        : server_core{ *this }
        , _communicator{ server_core }
    {

    }

    bool ChatServer::handle_message(const ::net::MessageRequest& request, ::net::MessageResponse& response)
    {
        if (auto handler = message_handler_db[request.message_id()])
            return (this->*handler)(request, response);

        CONSOLE_LOG(error) << "Unimplemented message id : " << request.message_id();
        return false;
    }

    bool ChatServer::handle_packet(const ::net::MessageRequest& request, ::net::MessageResponse& response)
    {
        ::net::PacketRequest packet_request(request);
        ::net::PacketResponse packet_response(response);

        if (auto handler = packet_handler_db[packet_request.packet_id()])
            return (this->*handler)(packet_request, packet_response);

        CONSOLE_LOG(error) << "Unimplemented packet id : " << packet_request.packet_id();
        return false;
    }

    void ChatServer::serve_forever(int argc, char* argv[])
    {
        // Initialization
        auto router_ip = argv[1];
        auto router_port = std::atoi(argv[2]);

        if (not ::config::load_remote_config(router_ip, router_port, protocol::ServerType::Chat, chat::config::get_config()))
            return;

        auto& conf = chat::config::get_config();
        logging::initialize_system(conf.log().log_dir(), conf.log().log_filename());

        _communicator.register_server(protocol::ServerType::Router, 
            {
                .ip = router_ip,
                .port = router_port
            }
        );

        // Start server.
        server_core.start_network_io_service(conf.server().ip(), conf.server().port(), conf.system().num_of_processors());

        while (1) {
            _communicator.announce_server(protocol::ServerType::Chat, {
                .ip = conf.server().ip(),
                .port = conf.server().port()
            });

            util::sleep_ms(3000);
        }
    }
}
}