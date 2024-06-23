#include "pch.h"
#include "master_server.h"

#include <array>
#include <cstring>

#include "config/config.h"
#include "proto/config.pb.h"
#include "logging/error.h"
#include "database/query.h"

namespace net
{
    MasterServer::MasterServer(const config::Configuration_Server& server_conf)
        : connection_env{ server_conf.max_player() }
        , io_service { io::DEFAULT_NUM_OF_CONCURRENT_EVENT_THREADS }
        , server_core{ *this, connection_env, io_service, server_conf }
        , database_core{ }
        , world{ connection_env }
        
        , deferred_handshake_packet_task{ &MasterServer::handle_deferred_handshake_packet, this, user_authentication_task_interval }
        , deferred_chat_message_packet_task{ &MasterServer::handle_deferred_chat_message_packet, this, chat_message_task_interval }
    {

    }

    error::ResultCode MasterServer::handle_packet(net::Connection::Descriptor& conn_descriptor, Packet* packet)
    {
        switch (packet->id) {
        case PacketID::Handshake:
            return handle_handshake_packet(conn_descriptor, *static_cast<PacketHandshake*>(packet));
        case PacketID::SetBlockClient:
            return handle_set_block_packet(conn_descriptor, *static_cast<PacketSetBlockClient*>(packet));
        case PacketID::SetPlayerPosition:
            return handle_player_position_packet(conn_descriptor, *static_cast<PacketSetPlayerPosition*>(packet));
        case PacketID::ChatMessage:
            return handle_chat_message_packet(conn_descriptor, *static_cast<PacketChatMessage*>(packet));
        default:
            CONSOLE_LOG(error) << "Unimplemented packet id: " << int(packet->id);
            return error::PACKET_UNIMPLEMENTED_ID;
        }
    }

    error::ResultCode MasterServer::handle_handshake_packet(net::Connection::Descriptor& conn_descriptor, net::PacketHandshake& packet)
    {
        deferred_handshake_packet_task.push_packet(conn_descriptor.connection_key(), packet);
        return error::PACKET_HANDLE_DEFERRED;
    }

    error::ResultCode MasterServer::handle_set_block_packet(net::Connection::Descriptor& conn_descriptor, net::PacketSetBlockClient& packet)
    {
        auto block_id = packet.mode == game::BlockMode::SET ? packet.block_id : game::BLOCK_AIR;
        if (not world.try_change_block({ packet.x, packet.y, packet.z }, block_id))
            goto REVERT_BLOCK;

        return error::SUCCESS;

    REVERT_BLOCK:
        block_id = packet.mode == game::BlockMode::SET ? game::BLOCK_AIR : packet.block_id;
        net::PacketSetBlockServer revert_block_packet({ packet.x, packet.y, packet.z }, block_id);
        conn_descriptor.send_packet(net::ThreadType::Any_Thread, revert_block_packet); 

        return error::SUCCESS;
    }

    error::ResultCode MasterServer::handle_player_position_packet(net::Connection::Descriptor& conn_descriptor, net::PacketSetPlayerPosition& packet)
    {
        if (auto player = conn_descriptor.get_connected_player())
            player->set_position(packet.player_pos);
        
        return error::PACKET_HANDLE_DEFERRED;
    }

    error::ResultCode MasterServer::handle_chat_message_packet(net::Connection::Descriptor& conn_descriptor, net::PacketChatMessage& packet)
    {
        if (auto player = conn_descriptor.get_connected_player()) {
            packet.player_id = player->game_id(); // client always sends 0xff(SELF ID).
            deferred_chat_message_packet_task.push_packet(conn_descriptor.connection_key(), packet);
            return error::PACKET_HANDLE_DEFERRED;
        }
        return error::PACKET_CHAT_MESSAGE_HANDLE_ERROR;
    }

    void MasterServer::tick()
    {
        Connection::Descriptor::flush_send(connection_env);
        Connection::Descriptor::flush_receive(connection_env);

        flush_deferred_packet();
    }

    void MasterServer::serve_forever()
    {
        const auto& db_conf = config::get_database_config();

        // start database system.
        if (not database_core.connect_with_password(db_conf))
            throw error::DATABASE_CONNECT;

        // start network I/O system.
        server_core.start_network_io_service();

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
        database::PlayerLoginSQL player_login{ database_core.get_connection_handle() };
        database::PlayerSearchSQL player_search{ database_core.get_connection_handle() };

        for (auto packet = packet_head; packet; packet = packet->next) {
            auto player_type = game::PlayerType::INVALID;

            if (not player_search.search(packet->username)) {
                player_type = std::strlen(packet->password) ? game::PlayerType::NEW_USER : game::PlayerType::GUEST;
            }
            else if (player_login.authenticate(packet->username, packet->password)) {
                player_type = std::strcmp(packet->username, "admin")
                    ? game::PlayerType::AUTHENTICATED_USER : game::PlayerType::ADMIN;
            }
            
            // return handshake result to clients.

            if (auto desc = connection_env.try_acquire_descriptor(packet->connection_key)) {
                if (player_type == game::PlayerType::INVALID) {
                    desc->send_disconnect_message(net::ThreadType::Any_Thread, error::PACKET_RESULT_FAIL_LOGIN);
                    continue;
                }

                auto player = world.add_player(
                        packet->connection_key,
                        player_search.get_player_identity(),
                        player_type,
                        packet->username,
                        packet->password);

                if (player == nullptr) {
                    desc->send_disconnect_message(net::ThreadType::Any_Thread, error::PACKET_RESULT_ALREADY_LOGIN);
                    continue;
                }

                desc->on_handshake_success(player);
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