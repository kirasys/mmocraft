#include "pch.h"
#include "master_server.h"

#include <array>
#include <cstring>

#include "config/config.h"
#include "logging/error.h"
#include "database/query.h"
#include "game/player_command.h"

namespace net
{
    MasterServer::MasterServer(net::ConnectionEnvironment& a_connection_env, io::IoCompletionPort& a_io_service)
        : connection_env{ a_connection_env }
        , io_service { a_io_service }
        , server_core{ *this, connection_env, io_service }
        , world{ connection_env, database_core }
        
        , deferred_handshake_packet_task{ &MasterServer::handle_deferred_handshake_packet, this, user_authentication_task_interval }
        , deferred_chat_message_packet_task{ &MasterServer::handle_deferred_chat_message_packet, this, chat_message_task_interval }
    {

    }

    error::ResultCode MasterServer::handle_packet(net::Connection& conn, Packet* packet)
    {
        switch (packet->id) {
        case PacketID::Handshake:
            return handle_handshake_packet(conn, *static_cast<PacketHandshake*>(packet));
        case PacketID::Ping:
            return handle_ping_packet(conn, *static_cast<PacketPing*>(packet));
        case PacketID::SetBlockClient:
            return handle_set_block_packet(conn, *static_cast<PacketSetBlockClient*>(packet));
        case PacketID::SetPlayerPosition:
            return handle_player_position_packet(conn, *static_cast<PacketSetPlayerPosition*>(packet));
        case PacketID::ChatMessage:
            return handle_chat_message_packet(conn, *static_cast<PacketChatMessage*>(packet));
        case PacketID::ExtInfo:
            return handle_ext_info_packet(conn, *static_cast<PacketExtInfo*>(packet));
        case PacketID::ExtEntry:
            return handle_ext_entry_packet(conn, *static_cast<PacketExtEntry*>(packet));
        default:
            CONSOLE_LOG(error) << "Unimplemented packet id: " << int(packet->id);
            return error::PACKET_UNIMPLEMENTED_ID;
        }
    }

    error::ResultCode MasterServer::handle_handshake_packet(net::Connection& conn, net::PacketHandshake& packet)
    {
        deferred_handshake_packet_task.push_packet(conn.connection_key(), packet);
        return error::PACKET_HANDLE_DEFERRED;
    }

    error::ResultCode MasterServer::handle_ping_packet(net::Connection& conn, net::PacketPing& packet)
    {
        // send pong.
        conn.io()->send_ping();
        return error::SUCCESS;
    }

    error::ResultCode MasterServer::handle_set_block_packet(net::Connection& conn, net::PacketSetBlockClient& packet)
    {
        auto block_id = packet.mode == game::BlockMode::SET ? packet.block_id : game::BLOCK_AIR;
        if (not world.try_change_block({ packet.x, packet.y, packet.z }, block_id))
            goto REVERT_BLOCK;

        return error::SUCCESS;

    REVERT_BLOCK:
        block_id = packet.mode == game::BlockMode::SET ? game::BLOCK_AIR : packet.block_id;
        net::PacketSetBlockServer revert_block_packet({ packet.x, packet.y, packet.z }, block_id);

        conn.io()->send_packet(revert_block_packet);

        return error::SUCCESS;
    }

    error::ResultCode MasterServer::handle_player_position_packet(net::Connection& conn, net::PacketSetPlayerPosition& packet)
    {
        if (auto player = conn.associated_player())
            player->set_position(packet.player_pos);
        
        return error::SUCCESS;
    }

    error::ResultCode MasterServer::handle_chat_message_packet(net::Connection& conn, net::PacketChatMessage& packet)
    {
        if (auto player = conn.associated_player()) {
            packet.player_id = player->game_id(); // client always sends 0xff(SELF ID).

            if (packet.message.data[0] == '/') { // if command message
                game::PlayerCommand command(player);
                command.execute(world, { packet.message.data, packet.message.size });

                net::PacketChatMessage msg_packet(command.get_response());
                conn.io()->send_packet(msg_packet);
;            }
            else {
                deferred_chat_message_packet_task.push_packet(conn.connection_key(), packet);
                return error::PACKET_HANDLE_DEFERRED;
            }
        }
        return error::SUCCESS;
    }

    error::ResultCode MasterServer::handle_ext_info_packet(net::Connection& conn, net::PacketExtInfo& packet)
    {
        if (auto player = conn.associated_player()) {
            player->set_extension_count(packet.extension_count);
        }

        return error::SUCCESS;
    }

    error::ResultCode MasterServer::handle_ext_entry_packet(net::Connection& conn, net::PacketExtEntry& packet)
    {
        if (auto player = conn.associated_player()) {
            std::string_view cpe_name = { packet.extenstion_name.data, packet.extenstion_name.size };
            if (net::is_cpe_support(cpe_name, packet.version))
                player->register_extension(net::cpe_index_of(cpe_name));

            if (player->decrease_pending_extension_count() == 0)
                conn.on_handshake_success();
        }

        return error::SUCCESS;
    }

    void MasterServer::tick()
    {
        ConnectionIO::flush_send(connection_env);
        ConnectionIO::flush_receive(connection_env);

        flush_deferred_packet();

        if (server_core.is_stopped())
            server_core.start_accept();
    }

    void MasterServer::serve_forever()
    {
        // start network I/O system.
        auto& server_conf = config::get_server_config();
        auto& system_conf = config::get_system_config();
        server_core.start_network_io_service(server_conf.ip(), server_conf.port(), system_conf.num_of_processors() * 2);

        // load world map.
        const auto& world_conf = config::get_world_config();

        world.load_filesystem_world(world_conf.save_dir());

        while (1) {
            std::size_t start_tick = util::current_monotonic_tick();

            this->tick();
            world.tick(io_service);

            std::size_t end_tick = util::current_monotonic_tick();

            if (auto diff = end_tick - start_tick; diff < 100)
                util::sleep_ms(std::max(100 - diff, std::size_t(30)));
        }
    }

    void MasterServer::flush_deferred_packet()
    {
        for (auto task : deferred_packet_tasks) {
            if (task->ready()) {
                io_service.schedule_task(task);
            }
        }
    }

    /**
     *  Deferred packet handler methods.
     */

    void MasterServer::handle_deferred_handshake_packet(io::Task* task, const DeferredPacket<net::PacketHandshake>* packet_head)
    {
        database::PlayerLoginSQL player_login;

        if (not player_login.is_valid()) {
            CONSOLE_LOG(error) << "Fail to allocate sql statement handles.";
            return;
        }

        for (auto packet = packet_head; packet; packet = packet->next) {
            player_login.authenticate(packet->username, packet->password);

            // return handshake result to clients.

            if (auto conn = connection_env.try_acquire_connection(packet->connection_key)) {
                if (player_login.player_type() == game::PlayerType::INVALID) {
                    conn->disconnect_with_message(error::PACKET_RESULT_FAIL_LOGIN);
                    continue;
                }

                if (world.is_already_exist_player(packet->username)) {
                    conn->disconnect_with_message(error::PACKET_RESULT_ALREADY_LOGIN);
                    continue;
                }

                // Associate player with connection and load game data from database.
                conn->associate_player(std::make_unique<game::Player>(
                        packet->connection_key,
                        player_login.player_identity(),
                        player_login.player_type(),
                        packet->username
                ));

                conn->associated_player()->load_gamedata(
                    player_login.player_gamedata(),
                    database::player_gamedata_column_size
                );

                // Register to world player table.
                world.register_player(packet->username, packet->connection_key);

                if (packet->cpe_support)
                    conn->send_supported_cpe_list();
                else
                    conn->on_handshake_success();
            }
        }
    }

    void MasterServer::handle_deferred_chat_message_packet(io::Task* task, const DeferredPacket<net::PacketChatMessage>* packet_head)
    {
        std::vector<const DeferredPacket<net::PacketChatMessage>*> packets;
        for (auto packet = packet_head; packet; packet = packet->next)
            packets.push_back(packet);

        std::unique_ptr<std::byte[]> packet_data;
        if (auto data_size = DeferredPacket<net::PacketChatMessage>::serialize(packets, packet_data)) {
            world.multicast_to_world_player(net::MuticastTag::Chat_Message, std::move(packet_data), data_size);
        }
    }
}