#include "chat_server.h"
#include "../config/config.h"

#include <util/time_util.h>
#include <logging/logger.h>

namespace chat
{
    ChatServer::ChatServer()
        : server_core{ *this }
        , _communicator{ server_core }
    {

    }

    bool ChatServer::handle_message(const net::MessageRequest&, net::MessageResponse& response)
    {
        return true;
    }

    void ChatServer::serve_forever(int argc, char* argv[])
    {
        // Initialization
        auto router_ip = argv[1];
        auto router_port = std::atoi(argv[2]);

        net::Socket::initialize_system();

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
            util::sleep_ms(3000);
        }
    }
}
