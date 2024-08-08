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

    void ChatServer::serve_forever()
    {
        auto& conf = chat::config::get_config();
        
        server_core.start_network_io_service(conf.server().ip(), conf.server().port(), conf.system().num_of_processors());

        while (1) {
            util::sleep_ms(3000);
        }
    }
}
