#include "chat_server.h"

#include <config/config.h>
#include <net/packet.h>
#include <util/time_util.h>
#include <logging/logger.h>

#include "../config/config.h"
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
        : server_core{ this, &message_handler_table }
        , interval_tasks{ this }
    {
        interval_tasks.schedule(
            ::util::TaskTag::ANNOUNCE_SERVER, 
            &ChatServer::announce_server, 
            ::util::MilliSecond(::config::announce_server_period_ms)
        );
    }

    bool ChatServer::handle_chat_packet(const ::net::PacketRequest& request, ::net::MessageResponse& response)
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
        return error::code::success;
    }

    bool ChatServer::initialize(const char* router_ip, int router_port)
    {
        auto& comm = server_core.communicator();
        comm.register_server(protocol::ServerType::Router, { router_ip, router_port });

        if (not comm.load_remote_config(protocol::ServerType::Chat, chat::config::get_config()))
            return false;

        
        auto& conf = chat::config::get_config();
        ::config::set_default_configuration(*conf.mutable_system());

        logging::initialize_system(conf.log().log_dir(), conf.log().log_filename());

        return true;
    }

    void ChatServer::serve_forever(int argc, char* argv[])
    {
        // Initialization
        auto router_ip = argv[1];
        auto router_port = std::atoi(argv[2]);
        if (not initialize(router_ip, router_port))
            return;

        // Start server.
        auto& conf = chat::config::get_config();
        server_core.start_network_io_service(conf.server().ip(), conf.server().port(), conf.system().num_of_processors());

        while (1) {
            std::size_t start_tick = util::current_monotonic_tick();

            interval_tasks.process_tasks();

            std::size_t end_tick = util::current_monotonic_tick();

            if (auto diff = end_tick - start_tick; diff < 300)
                util::sleep_ms(std::max(300 - diff, std::size_t(100)));
        }
    }
}
}