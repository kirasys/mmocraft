#include "pch.h"
#include "world.h"

#include <algorithm>
#include <bitset>
#include <cstring>
#include <iostream>

#include "database/database_core.h"
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
    World::World(net::ConnectionEnvironment& a_connection_env, database::DatabaseCore& db_core)
        : connection_env{ a_connection_env }
        , database_core{ db_core }
        , spawn_player_task{ &World::spawn_player, this, spawn_player_task_interval }
        , disconnect_player_task{ &World::disconnect_player, this, despawn_player_task_interval }
        , sync_block_task{ &World::sync_block, this, sync_block_data_task_interval }
        , sync_player_position_task{ &World::sync_player_position, this, sync_player_position_task_interval }
    {
        player_lookup_table.reserve(connection_env.size_of_max_connections());
    }
    /*
    net::Connection* World::try_acquire_player_connection(const char* key)
    {
        std::shared_lock lock(player_lookup_table_mutex);
        return player_lookup_table.find(key) != player_lookup_table.end() ?
            connection_env.try_acquire_connection(player_lookup_table.at(key)) : nullptr;
    }

    void World::unicast_to_world_player(const char* username, net::MessageType message_type, const char* message)
    {
        if (auto conn = try_acquire_player_connection(username)) {
            net::PacketExtMessage message_packet(message_type, message);
            conn->io()->send_packet(message_packet);
        }
    }
    */
    void World::broadcast_to_world_player(net::MessageType message_type, const char* message)
    {
        std::vector<game::Player*> world_players;
        world_players.reserve(connection_env.size_of_max_connections());

        connection_env.select_players([](const game::Player* player)
            { return player->state() >= PlayerState::Spawned; },
            world_players);

        net::PacketExtMessage message_packet(message_type, message);

        for (auto player : world_players) {
            if (auto conn = connection_env.try_acquire_connection(player->connection_key())) {
                conn->io()->send_packet(message_packet);
            }
        }
    }

    void World::send_to_players(const std::vector<game::Player*>& players, const std::byte* data, std::size_t data_size,
                                void(*successed)(game::Player*), void(*failed)(game::Player*))
    {
        for (auto player : players) {
            if (auto connection_io = connection_env.try_acquire_connection_io(player->connection_key())) {
                if (connection_io->send_raw_data(data, data_size))
                    if (successed) successed(player);
                else
                    if (failed) failed(player);
            }
        }
    }

    void World::multicast_to_players(const std::vector<game::Player*>& players, io::MulticastDataEntry& entry, void(*successed)(game::Player*))
    {
        for (auto player : players) {
            if (auto connection_io = connection_env.try_acquire_connection_io(player->connection_key())) {
                connection_io->send_multicast_data(entry);
                // multicast fails only fatal situation.
                if (successed) successed(player);
            }
        }

        entry.set_reference_count_mode(true);
    }

    void World::process_level_wait_player(const std::vector<game::Player*>& level_wait_players)
    {
        if (last_level_data_submission_at + level_data_submission_interval > util::current_monotonic_tick())
            return;

        // compress and serialize block datas.
        net::PacketLevelDataChunk level_packet(block_mapping.data(), _metadata.volume(),
            net::PacketFieldType::Short(_metadata.width()),
            net::PacketFieldType::Short(_metadata.height()),
            net::PacketFieldType::Short(_metadata.length())
        );

        std::unique_ptr<std::byte[]> level_packet_data;
        auto data_size = level_packet.serialize(level_packet_data);

        auto& data_entry = multicast_manager.set_data(io::MulticastTag::Level_Data, std::move(level_packet_data), data_size);
        multicast_to_players(level_wait_players, data_entry, [](game::Player* player) {
            player->set_state(game::PlayerState::Level_Initialized);
        });
        
        last_level_data_submission_at = util::current_monotonic_tick();
    }

    void World::spawn_player(const std::vector<game::Player*>& spawn_wait_players)
    {
        // get existing all players in the world.
        std::vector<game::Player*> old_players;
        old_players.reserve(connection_env.size_of_max_connections());

        connection_env.select_players([](const game::Player* player)
            { return player->state() >= PlayerState::Spawned; },
            old_players);

        // create spawn packet of all players.
        std::unique_ptr<std::byte[]> spawn_packet_data;
        auto spawn_packet_data_size = net::PacketSpawnPlayer::serialize(old_players, spawn_wait_players, spawn_packet_data);

        // spawn all players to the new players.
        send_to_players(spawn_wait_players, spawn_packet_data.get(), spawn_packet_data_size,
            [](game::Player* player) {
                player->set_state(game::PlayerState::Spawned);
            });

        // spawn new player to the old players. 
        auto new_players_spawn_packet_data = spawn_packet_data.get() + old_players.size() * net::PacketSpawnPlayer::packet_size;
        send_to_players(old_players, new_players_spawn_packet_data, spawn_wait_players.size() * net::PacketSpawnPlayer::packet_size);
    }

    void World::despawn_player(const std::vector<game::Player*>& despawn_wait_players)
    {
        // create despawn packets
        std::unique_ptr<std::byte[]> despawn_packet_data;
        auto data_size = net::PacketDespawnPlayer::serialize(despawn_wait_players, despawn_packet_data);

        send_to_specific_players<PlayerState::Spawned>(despawn_packet_data.get(), data_size);
    }

    void World::disconnect_player(const std::vector<game::Player*>& disconnect_wait_players)
    {
        despawn_player(disconnect_wait_players);

        // Update player game data then set offline.
        database::PlayerUpdateSQL player_update_sql;

        for (auto player : disconnect_wait_players) {
            if (not player_update_sql.update(*player))
                CONSOLE_LOG(error) << "Fail to update player data.";

            if (auto conn = connection_env.try_acquire_connection(player->connection_key())) {
                conn->disconnect();
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

    void World::commit_block_changes(const std::byte* block_history_data, std::size_t data_size)
    {
        auto history_size = data_size / game::BlockHistory<>::history_data_unit_size;
        auto block_array = block_mapping.data();
        
        for (std::size_t index = 0; index < history_size; index++) {
            auto& record = game::BlockHistory<>::get_record(block_history_data, index);
            std::size_t block_map_index = coordinate_to_block_map_index(record.x, record.y, record.z);

            if (block_map_index < _metadata.volume())
                block_array[block_map_index] = record.block_id;
        }
    }

    void World::sync_block(const std::vector<game::Player*>& level_wait_players, game::BlockHistory<>& block_history)
    {
        // serialize and fetch block histories.
        std::unique_ptr<std::byte[]> block_history_data;
        if (std::size_t data_size = block_history.fetch_serialized_data(block_history_data)) {
            commit_block_changes(block_history_data.get(), data_size);

            auto& data_entry = multicast_manager.set_data(io::MulticastTag::Sync_Block_Data, std::move(block_history_data), data_size);
            multicast_to_specific_players< PlayerState::Level_Initialized>(data_entry);
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
            { return player->state() >= PlayerState::Spawned; },
            world_players);

        // create set player position packets.
        std::unique_ptr<std::byte[]> position_packet_data;
        if (auto data_size = net::PacketSetPlayerPosition::serialize(world_players, position_packet_data)) {
            auto& data_entry = multicast_manager.set_data(io::MulticastTag::Sync_Player_Position, std::move(position_packet_data), data_size);
            multicast_to_players(world_players, data_entry, [](game::Player* player) {
                player->commit_last_transferrd_position();
            });
        }
    }

    bool World::try_change_block(util::Coordinate3D pos, BlockID block_id)
    {
        return sync_block_task.push(pos, block_id);
    }

    void World::tick(io::RegisteredIO& task_scheduler)
    {
        auto transit_player_state = [this](net::Connection& conn, game::Player& player) {
            switch (player.state()) {
            case game::PlayerState::Handshake_Completed:
            {
                net::PacketSetPlayerID set_player_id_packet(player.game_id());
                if (conn.io()->send_packet(set_player_id_packet)) {
                    sync_block_task.push(&player);
                    player.set_state(PlayerState::Level_Wait);
                }
            }
            break;
            case game::PlayerState::Level_Initialized:
            {
                player.set_spawn_coordinate(_metadata.spawn_x(), _metadata.spawn_y(), _metadata.spawn_z(), false);
                player.set_spawn_orientation(_metadata.spawn_yaw(), _metadata.spawn_pitch(), false);
                player.set_state(PlayerState::Spawn_Wait);

                spawn_player_task.push(&player);
            }
            break;
            case game::PlayerState::Spawned:
            {
                if (player.last_ping_time() + ping_interval < util::current_monotonic_tick()) {
                    conn.io()->send_ping();
                    player.update_ping_time();
                }
            }
            break;
            case game::PlayerState::Disconnect_Wait:
            {
                disconnect_player_task.push(&player);
                player.set_state(PlayerState::Disconnect_Completed);
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