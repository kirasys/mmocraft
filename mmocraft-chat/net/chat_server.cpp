#include "chat_server.h"

#include <net/packet.h>

#include <config/config.h>
#include <util/time_util.h>
#include <logging/logger.h>

#include "../config/config.h"

namespace chat
{
namespace net
{
    ChatServer::ChatServer()
        : server_core{ *this }
        , interval_tasks{ this }
    {
        interval_tasks.schedule(
            ::util::interval_task_tag_id::announce_server, 
            &ChatServer::announce_server, 
            ::util::MilliSecond(::config::task::announce_server_period)
        );
    }

    bool ChatServer::handle_message(::net::MessageRequest& request)
    {
        switch (request.message_id()) {
        case ::net::message_id::chat_command:
            handle_chat_command(request);
            return true;
        default:
            return false;
        }
    }

    database::AsyncTask ChatServer::handle_chat_command(::net::MessageRequest& request)
    {
        protocol::ChatCommandRequest msg;
        if (not request.parse_message(msg))
            co_return;

        if (msg.message()[0] != '/') { // if common chat just logging.
            database::collection::ChatMessage chat_msg;
            chat_msg.message = msg.message();
            chat_msg.sender_name = msg.sender_player_name();

            co_await database::CouchbaseCore::insert_document(database::CollectionPath::chat_message_common, chat_msg);
            co_return;
        }


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
    }

    bool ChatServer::initialize(const char* router_ip, int router_port)
    {
        auto& comm = server_core.communicator();
        comm.register_server(protocol::server_type_id::router, { router_ip, router_port });

        if (not comm.load_remote_config(protocol::server_type_id::chat, chat::config::get_config()))
            return false;

        
        auto& conf = chat::config::get_config();
        ::config::set_default_configuration(*conf.mutable_system());

        logging::initialize_system(conf.log().log_dir(), conf.log().log_filename());

        return true;
    }

    void ChatServer::announce_server()
    {
        auto& conf = config::get_config();

        if (not server_core.communicator().announce_server(server_type, {
            .ip = conf.server().ip(),
            .port = conf.server().port()
            })) {
            CONSOLE_LOG(error) << "Fail to announce chat server";
        }
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