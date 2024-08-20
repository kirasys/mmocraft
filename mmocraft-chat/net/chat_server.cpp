#include "chat_server.h"
#include "../config/config.h"

#include <config/config.h>
#include <net/packet.h>
#include <util/time_util.h>
#include <logging/logger.h>

namespace
{
    std::array<chat::net::ChatServer::handler_type, 0x100> message_handler_table = [] {
        std::array<chat::net::ChatServer::handler_type, 0x100> arr{};
        return arr;
    }();

    std::array<chat::net::ChatServer::packet_handler_type, 0x100> packet_handler_table = [] {
        std::array<chat::net::ChatServer::packet_handler_type, 0x100> arr{};
        arr[::net::PacketID::ChatMessage] = &chat::net::ChatServer::handle_chat_packet;
        return arr;
    }();
}

namespace chat
{
namespace net
{
    ChatServer::ChatServer()
        : server_core{ this, &message_handler_table, &packet_handler_table }
    {

    }

    bool ChatServer::handle_chat_packet(const ::net::PacketRequest& request, ::net::PacketResponse& response)
    {
        ::net::PacketChatMessage packet(request.packet_data());

        /*
        if (auto player = conn.associated_player()) {
            packet.player_id = player->game_id(); // client always sends 0xff(SELF ID).

            if (packet.message[0] == '/') { // if command message
                game::PlayerCommand command(player);
                command.execute(world, packet.message);

                net::PacketChatMessage msg_packet(command.get_response());
                conn.io()->send_packet(msg_packet);
                ;
            }
            else {
                deferred_chat_message_packet_task.push_packet(conn.connection_key(), packet);
                return error::PACKET_HANDLE_DEFERRED;
            }
        }
        */
        return error::SUCCESS;
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

        server_core.communicator().register_server(protocol::ServerType::Router,
            {
                .ip = router_ip,
                .port = router_port
            }
        );

        // Start server.
        server_core.start_network_io_service(conf.server().ip(), conf.server().port(), conf.system().num_of_processors());

        while (1) {
            server_core.communicator().announce_server(protocol::ServerType::Chat, {
                .ip = conf.server().ip(),
                .port = conf.server().port()
            });

            util::sleep_ms(3000);
        }
    }
}
}