#include "pch.h"
#include "world.h"

#include <algorithm>
#include <bitset>
#include <cstring>
#include <iostream>

#include "database/query.h"
#include "net/packet_extension.h"
#include "net/connection.h"
#include "net/connection_environment.h"
#include "game/world_generator.h"

#include "logging/logger.h"
#include "proto/generated/world_metadata.pb.h"
#include "util/time_util.h"
#include "util/protobuf_util.h"

namespace fs = std::filesystem;

namespace game
{
    World::World(net::ConnectionEnvironment& a_connection_env)
        : connection_env{ a_connection_env }
        , spawn_player_task{ &World::spawn_player, this, game::world_task_interval::spawn_player }
        , disconnect_player_task{ &World::disconnect_player, this, game::world_task_interval::despawn_player }
        , sync_block_task{ &World::sync_block, this, game::world_task_interval::sync_block }
        , sync_player_position_task{ &World::sync_player_position, this, game::world_task_interval::sync_player_position }
        , common_chat_transfer_task{ &World::common_chat_transfer, this, game::world_task_interval::common_chat_transfer }
    { }

    void World::broadcast_to_world_player(net::chat_message_type_id message_type, const char* message)
    {
        std::vector<game::Player*> world_players;
        world_players.reserve(connection_env.size_of_max_connections());

        connection_env.select_players([](const game::Player* player)
            { return player->state() >= PlayerState::spawned; },
            world_players);

        net::PacketExtMessage message_packet(message_type, message);

        for (auto player : world_players) {
            if (auto conn = connection_env.try_acquire_connection(player->connection_key())) {
                conn->io()->send_packet(message_packet);
            }
        }
    }

    void World::send_to_players(const std::vector<game::Player*>& players, util::byte_view data,
                                void(*successed)(game::Player*), void(*failed)(game::Player*))
    {
        for (auto player : players) {
            if (auto connection_io = connection_env.try_acquire_connection_io(player->connection_key())) {
                if (connection_io->send_raw_data(data.data(), data.size()))
                    if (successed) successed(player);
                else
                    if (failed) failed(player);
            }
        }
    }

    void World::multicast_to_players(const std::vector<game::Player*>& players, std::shared_ptr<io::IoMulticastEventData>& data, void(*successed)(game::Player*))
    {
        for (auto player : players) {
            if (auto connection_io = connection_env.try_acquire_connection_io(player->connection_key())) {
                if (connection_io->post_multicast_event(data) && successed) successed(player);
            }
        }
    }

    /* World task start */

    void World::process_level_wait_player(const std::vector<game::Player*>& level_wait_players)
    {
        // compress and serialize block datas.
        net::PacketLevelDataChunk level_packet(block_mapping.data(), _metadata.volume(),
            net::PacketFieldType::Short(_metadata.width()),
            net::PacketFieldType::Short(_metadata.height()),
            net::PacketFieldType::Short(_metadata.length())
        );

        std::unique_ptr<std::byte[]> level_packet_data;
        auto data_size = level_packet.serialize(level_packet_data);

        auto multicast_data = std::make_shared<io::IoMulticastEventData>(std::move(level_packet_data), data_size);
        multicast_to_players(level_wait_players, multicast_data, [](game::Player* player) {
            player->transit_state(game::PlayerState::level_initialized);
        });
    }

    void World::spawn_player(const std::vector<game::Player*>& spawn_wait_players)
    {
        // get existing all players in the world.
        std::vector<game::Player*> old_players;
        old_players.reserve(connection_env.size_of_max_connections());

        connection_env.select_players([](const game::Player* player)
            { return player->state() >= PlayerState::spawned; },
            old_players);

        // create spawn packet of all players.
        std::unique_ptr<std::byte[]> spawn_packet_data;
        auto spawn_packet_data_size = net::PacketSpawnPlayer::serialize(old_players, spawn_wait_players, spawn_packet_data);

        // spawn all players to the new players.
        send_to_players(spawn_wait_players, { spawn_packet_data.get(), spawn_packet_data_size },
            [](game::Player* player) {
                player->transit_state(game::PlayerState::spawned);
            });

        // spawn new player to the old players. 
        auto new_players_spawn_packet_data = spawn_packet_data.get() + old_players.size() * net::PacketSpawnPlayer::packet_size;
        send_to_players(old_players, { new_players_spawn_packet_data, spawn_wait_players.size() * net::PacketSpawnPlayer::packet_size });
    }

    void World::despawn_player(const std::vector<game::Player*>& spawned_players)
    {
        // create despawn packets
        std::unique_ptr<std::byte[]> despawn_packet_data;
        auto data_size = net::PacketDespawnPlayer::serialize(spawned_players, despawn_packet_data);

        send_to_specific_players<PlayerState::spawned>({ despawn_packet_data.get(), data_size });
    }

    void World::disconnect_player(const std::vector<game::Player*>& disconnect_wait_players)
    {
        // Send despawn packet to world players.
        std::vector<game::Player*> spawned_players;
        for (auto player : disconnect_wait_players) {
            if (player->prev_state() >= game::PlayerState::spawned) {
                spawned_players.push_back(player);
            }
        }

        despawn_player(spawned_players);

        // Update player game data then set offline.
        for (auto player : disconnect_wait_players) {
            if (auto conn = connection_env.try_acquire_connection(player->connection_key())) {
                database::PlayerGamedata::save(*player);
                player->transit_state(game::PlayerState::disconnected);
            }
        }
    }

    std::size_t World::coordinate_to_block_map_index(int x, int y, int z)
    {
        return WorldGenerator::block_file_header_size
                + _byteswap_ushort(x)
                + _metadata.width() * _byteswap_ushort(z)
                + _metadata.width() * _metadata.length() * _byteswap_ushort(y);
    }

    void World::commit_block_changes(const game::BlockHistory& block_change_history)
    {
        const auto history_size = block_change_history.size();
        auto block_array = block_mapping.data();
        
        for (std::size_t index = 0; index < history_size; index++) {
            auto& record = block_change_history.get_record(index);
            std::size_t block_map_index = coordinate_to_block_map_index(record.x, record.y, record.z);

            if (block_map_index < _metadata.volume())
                block_array[block_map_index] = record.block_id;
        }
    }

    void World::sync_block(const std::vector<game::Player*>& level_wait_players, const game::BlockHistory& block_change_history)
    {
        // copy block histories.
        if (block_change_history.size()) {
            commit_block_changes(block_change_history);

            auto block_history_data = block_change_history.get_snapshot_data();
            std::unique_ptr<std::byte[]> copyed_block_history_data(block_history_data.clone());

            auto multicast_data = std::make_shared<io::IoMulticastEventData>(std::move(copyed_block_history_data), block_history_data.size());
            multicast_to_specific_players<PlayerState::level_initialized>(multicast_data);
        }
        
        // submit level data to handshaked players.
        // Note: level data task can't be seperated from block data task to achieve block data synchronization.
        //       the client dose not allow to send block datas before level initialization completed.
        if (not level_wait_players.empty())
            process_level_wait_player(level_wait_players);
    }

    void World::sync_player_position()
    {
        // get existing all players in the world.
        std::vector<game::Player*> world_players;
        world_players.reserve(connection_env.size_of_max_connections());

        connection_env.select_players([](const game::Player* player)
            { return player->state() >= PlayerState::spawned; },
            world_players);

        // create set player position packets.
        std::unique_ptr<std::byte[]> position_packet_data;
        if (auto data_size = net::PacketSetPlayerPosition::serialize(world_players, position_packet_data)) {
            auto multicast_data = std::make_shared<io::IoMulticastEventData>(std::move(position_packet_data), data_size);
            multicast_to_players(world_players, multicast_data, [](game::Player* player) {
                player->commit_last_transferrd_position();
            });
        }
    }

    void World::common_chat_transfer(util::byte_view chat_history_data)
    {
        if (std::size_t data_size = chat_history_data.size()) {
            std::unique_ptr<std::byte[]> copyed_chat_history_data(chat_history_data.clone());

            auto multicast_data = std::make_shared<io::IoMulticastEventData>(std::move(copyed_chat_history_data), data_size);
            multicast_to_specific_players<PlayerState::spawned>(multicast_data);
        }
    }

    /* World task end */

    bool World::try_change_block(util::Coordinate3D pos, BlockID block_id)
    {
        return sync_block_task.push(pos, block_id);
    }

    bool World::try_add_common_chat(util::byte_view chat_packet_data)
    {
        return common_chat_transfer_task.push(chat_packet_data);
    }

    void World::tick(io::RegisteredIO& task_scheduler)
    {
        auto transit_player_state = [this](net::Connection& conn, game::Player& player) {
            switch (player.state()) {
            case game::PlayerState::ex_handshaked:
            {
                net::PacketExtInfo ext_info_packet;
                net::PacketExtEntry ext_entry_packet;

                if (conn.io()->send_packet(ext_info_packet) && conn.io()->send_packet(ext_entry_packet))
                    player.prepare_state_transition(game::PlayerState::extension_syncing, game::PlayerState::extension_synced);
            }
            break;
            case game::PlayerState::handshaked:
            case game::PlayerState::extension_synced:
            {
                conn.on_handshake_success();

                player.prepare_state_transition(game::PlayerState::level_initializing, game::PlayerState::level_initialized);
                sync_block_task.push(&player);
            }
            break;
            case game::PlayerState::level_initialized:
            {
                player.set_spawn_coordinate(_metadata.spawn_x(), _metadata.spawn_y(), _metadata.spawn_z(), false);
                player.set_spawn_orientation(_metadata.spawn_yaw(), _metadata.spawn_pitch(), false);

                player.prepare_state_transition(game::PlayerState::spawning, game::PlayerState::spawned);
                spawn_player_task.push(&player);
            }
            break;
            case game::PlayerState::spawned:
            {
                if (player.last_ping_time() + game::world_task_interval::ping < util::current_monotonic_tick()) {
                    conn.io()->send_ping();
                    player.update_ping_time();
                }
            }
            break;
            case game::PlayerState::disconnecting:
            {
                disconnect_player_task.push(&player);
            }
            break;
            case game::PlayerState::disconnected:
            {
                conn.disconnect();
            }
            break;
            }
        };

        connection_env.for_each_player(transit_player_state);

        for (auto world_task : world_tasks) {
            if (world_task->ready()) {
                task_scheduler.schedule_task(world_task);
            }
        }
    }

    bool World::load_filesystem_world(std::string_view a_save_dir)
    {
        if (last_save_map_at) {
            CONSOLE_LOG(error) << "World is already loaded.";
            return false;
        }

        // set world files path.
        save_dir = a_save_dir;
        block_data_path = save_dir / block_data_filename;
        metadata_path = save_dir / world_metadata_filename;
        
        WorldGenerator::create_world_if_not_exist(block_data_path, metadata_path);

        load_metadata();
        load_block_data();

        last_save_map_at = util::current_monotonic_tick();

        return true;
    }
    
    void World::load_metadata()
    {
        util::json_file_to_proto_message(&_metadata, metadata_path);
    }

    void World::load_block_data()
    {
        if (not block_mapping.open(block_data_path.string())) {
            CONSOLE_LOG(error) << "Unalbe to mapping block map";
            return;
        }
    }
}