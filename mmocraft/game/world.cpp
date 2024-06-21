#include "pch.h"
#include "world.h"

#include <algorithm>
#include <bitset>
#include <cstring>
#include <iostream>


#include "net/packet.h"
#include "net/connection.h"
#include "net/connection_environment.h"
#include "game/world_generator.h"

#include "logging/logger.h"
#include "config/config.h"
#include "proto/config.pb.h"
#include "proto/world_metadata.pb.h"
#include "util/time_util.h"
#include "util/protobuf_util.h"

namespace fs = std::filesystem;

namespace game
{
    World::World(net::ConnectionEnvironment& a_connection_env)
        : connection_env{ a_connection_env }
        , multicast_manager{ a_connection_env }
        , players(connection_env.size_of_max_connections())
        , spawn_player_task{ &World::spawn_player, this, spawn_player_task_interval }
        , sync_player_position_task{ &World::sync_player_position, this, sync_player_position_task_interval }
    {

    }

    game::Player* World::add_player(net::ConnectionKey connection_key, 
                            unsigned player_identity,
                            game::PlayerType player_type,
                            const char* username,
                            const char* password)
    {
        bool is_already_logged_in = std::any_of(players.begin(), players.end(),
            [player_identity](std::unique_ptr<game::Player>& player) {
                return player_identity && player && player->identity() == player_identity;
            }
        );

        game::Player* player = nullptr;

        if (not is_already_logged_in) {
            player = new game::Player(
                connection_key,
                player_identity,
                player_type,
                username,
                password
            );

            players[connection_key.index()].reset(player);

            // prepare compressed block data for new player.
            caching_compressed_block_data();
        }

        return player;
    }

    void World::caching_compressed_block_data()
    {
        if (block_data_cache_expired_at > util::current_monotonic_tick())
            return;

        block_data_cache_expired_at = block_data_cache_lifetime + util::current_monotonic_tick();

        // compress and serialize block datas.
        net::PacketLevelDataChunk level_packet(block_mapping.data(), _metadata.volume(),
            net::PacketFieldType::Short(_metadata.width()),
            net::PacketFieldType::Short(_metadata.height()),
            net::PacketFieldType::Short(_metadata.length())
        );

        std::unique_ptr<std::byte[]> serialized_level_packet;
        auto packet_size = level_packet.serialize(serialized_level_packet);

        multicast_manager.set_multicast_data(net::MuticastTag::Level_Data, std::move(serialized_level_packet), packet_size);
    }

    void World::spawn_player()
    {
        // get new entered players.
        std::vector<game::Player*> new_players;

        auto spawn_wait_player_ptr = spawn_wait_players.pop();
        for (auto player_node = spawn_wait_player_ptr.get(); player_node; player_node = player_node->next)
            new_players.push_back(player_node->value);

        // get existing all players in the world.
        std::vector<game::Player*> old_players;
        old_players.reserve(connection_env.size_of_max_connections());

        connection_env.select_players([](const game::Player* player)
            { return player->state() > PlayerState::Spawn_Wait; },
            old_players);

        // create spawn packet of all players.
        std::unique_ptr<std::byte[]> spawn_packet_data;
        net::PacketSpawnPlayer::serialize(old_players, new_players, spawn_packet_data);

        // spawn new player for the existing players. 
        auto new_player_spawn_packet_data = spawn_packet_data.get() + old_players.size() * net::PacketSpawnPlayer::packet_size;

        for (auto player : old_players) {
            if (auto desc = connection_env.try_acquire_descriptor(player->connection_key())) {
                desc->send_raw_data(
                    net::ThreadType::Any_Thread,
                    new_player_spawn_packet_data,
                    new_players.size() * net::PacketSpawnPlayer::packet_size
                );
            }
        }

        // spawn all players for the new players. (use multicast)
        multicast_manager.set_multicast_data(
            net::MuticastTag::Spawn_Player,
            std::move(spawn_packet_data),
            (old_players.size() + new_players.size()) * net::PacketSpawnPlayer::packet_size
        );

        for (auto player : new_players) {
            auto desc = connection_env.try_acquire_descriptor(player->connection_key());
            if (desc && multicast_manager.send(net::MuticastTag::Spawn_Player, *desc))
                player->set_state(game::PlayerState::Spawned);
        }
    }

    void World::sync_player_position()
    {
        std::vector<game::Player*> world_players;
        world_players.reserve(connection_env.size_of_max_connections());

        connection_env.select_players([](const game::Player* player)
            { return player->state() >= PlayerState::Spawned; },
            world_players);

        std::unique_ptr<std::byte[]> position_packet_data(
            new std::byte[world_players.size() * net::PacketSetPlayerPosition::packet_size]
        );
        auto data_ptr = position_packet_data.get();

        for (auto player : world_players) {
            auto latest_pos = player->last_position();
            const auto diff = latest_pos - player->last_synced_position();

            player->start_sync_position(latest_pos);

            // absolute move position
            if (std::abs(diff.view.x) > 127 || std::abs(diff.view.y) > 127 || std::abs(diff.view.y) > 127) {
                *data_ptr++ = std::byte(net::PacketID::SetPlayerPosition);
                *data_ptr++ = std::byte(player->game_id());
                net::PacketStructure::write_short(data_ptr, latest_pos.view.x);
                net::PacketStructure::write_short(data_ptr, latest_pos.view.y);
                net::PacketStructure::write_short(data_ptr, latest_pos.view.z);
                *data_ptr++ = std::byte(latest_pos.view.yaw);
                *data_ptr++ = std::byte(latest_pos.view.pitch);
            }
            // relative move position
            else if (diff.raw_coordinate() && diff.raw_orientation()) {
                *data_ptr++ = std::byte(net::PacketID::UpdatePlayerPosition);
                *data_ptr++ = std::byte(player->game_id());
                *data_ptr++ = std::byte(diff.view.x);
                *data_ptr++ = std::byte(diff.view.y);
                *data_ptr++ = std::byte(diff.view.z);
                *data_ptr++ = std::byte(latest_pos.view.yaw);
                *data_ptr++ = std::byte(latest_pos.view.pitch);
            }
            // relative move coordinate
            else if (diff.raw_coordinate()) {
                *data_ptr++ = std::byte(net::PacketID::UpdatePlayerCoordinate);
                *data_ptr++ = std::byte(player->game_id());
                *data_ptr++ = std::byte(diff.view.x);
                *data_ptr++ = std::byte(diff.view.y);
                *data_ptr++ = std::byte(diff.view.z);
            }
            // relative move orientation
            else if (diff.raw_orientation()) {
                *data_ptr++ = std::byte(net::PacketID::UpdatePlayerOrientation);
                *data_ptr++ = std::byte(player->game_id());
                *data_ptr++ = std::byte(latest_pos.view.yaw);
                *data_ptr++ = std::byte(latest_pos.view.pitch);
            }
        }

        if (auto data_size = data_ptr - position_packet_data.get()) {
            multicast_manager.set_multicast_data(
                net::MuticastTag::Sync_Player_Position,
                std::move(position_packet_data),
                data_size
            );

            for (auto player : world_players) {
                auto desc = connection_env.try_acquire_descriptor(player->connection_key());
                if (desc && multicast_manager.send(net::MuticastTag::Sync_Player_Position, *desc))
                    player->end_sync_position();
            }

        }
    }

    void World::tick(io::IoCompletionPort& task_scheduler)
    {
        auto transit_player_state = [this](net::Connection::Descriptor& desc, game::Player& player) {
            switch (player.state()) {
            case game::PlayerState::Handshake_Completed:
            {
                net::PacketSetPlayerID packet(player.game_id());
                if (desc.send_packet(net::ThreadType::Tick_Thread, packet)) {
                    player.set_state(PlayerState::PlayerID_Tranferred);
                }
            }
            break;
            case game::PlayerState::PlayerID_Tranferred:
            {
                if (multicast_manager.send(net::MuticastTag::Level_Data, desc))
                    player.set_state(PlayerState::Level_Initialized);
            }
            break;
            case game::PlayerState::Level_Initialized:
            {
                player.set_default_spawn_coordinate(_metadata.spawn_x(), _metadata.spawn_y(), _metadata.spawn_z());
                player.set_default_spawn_orientation(_metadata.spawn_yaw(), _metadata.spawn_pitch());
                player.set_state(PlayerState::Spawn_Wait);

                spawn_wait_players.push(&player);
            }
            break;
            }
        };

        connection_env.for_each_player(transit_player_state);

        if (not spawn_wait_players.empty() && spawn_player_task.ready())
            task_scheduler.schedule_task(&spawn_player_task);

        if (sync_player_position_task.ready())
            task_scheduler.schedule_task(&sync_player_position_task);
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