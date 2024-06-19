#include "pch.h"
#include "world.h"

#include <algorithm>
#include <bitset>
#include <cstring>
#include <iostream>

#include "logging/logger.h"
#include "config/config.h"
#include "proto/config.pb.h"
#include "proto/world_metadata.pb.h"
#include "net/packet.h"
#include "net/connection.h"
#include "net/connection_environment.h"
#include "util/time_util.h"
#include "util/protobuf_util.h"

#define WORLD_METADATA_FORMAT_VERSION 1

namespace fs = std::filesystem;

namespace game
{
    World::World(net::ConnectionEnvironment& a_connection_env)
        : connection_env{ a_connection_env }
        , multicast_manager{ a_connection_env }
        , players(connection_env.size_of_max_connections())
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

    void World::tick()
    {
        auto transit_player_state = [this](net::Connection::Descriptor& desc, game::Player& player) {
            switch (player.state()) {
            case game::PlayerState::Handshake_Completed:
            {
                if (multicast_manager.send(net::MuticastTag::Level_Data, desc))
                    player.set_state(PlayerState::Level_Initialized);
                break;
            }
            case game::PlayerState::Level_Initialized:
            {
                net::PacketSetPlayerID packet(player.game_id());
                if (desc.send_packet(net::ThreadType::Tick_Thread, packet))
                    player.set_state(PlayerState::Assigned_PlayerID);
            }
            break;
            }
        };

        connection_env.for_each_player(transit_player_state);
    }

    bool World::load_filesystem_world(std::string_view a_save_dir)
    {
        if (last_saved_tick) {
            CONSOLE_LOG(error) << "World is already loaded.";
            return false;
        }

        // set world files path.
        save_dir = a_save_dir;
        block_data_path = save_dir / block_data_filename;
        world_metadata_path = save_dir / world_metadata_filename;
        
        if (not fs::exists(world_metadata_path))
            create_new_world();

        load_metadata();
        load_block_data();

        return true;
    }
    
    void World::load_metadata()
    {
        util::json_file_to_proto_message(&_metadata, world_metadata_path);
    }

    void World::load_block_data()
    {
        if (not block_mapping.open(block_data_path.string())) {
            CONSOLE_LOG(error) << "Unalbe to mapping block map";
            return;
        }
    }

    void World::create_new_world() const
    {
        const auto& world_conf = config::get_world_config();

        auto map_size = Coordinate3D{ short(world_conf.width()), short(world_conf.height()), short(world_conf.length())};
        unsigned long map_volume = map_size.x * map_size.y * map_size.z;
        
        // write world files to the disk.
        create_block_file(map_size, map_volume);
        create_metadata_file(map_size);
    }

    void World::create_block_file(Coordinate3D map_size, unsigned long map_volume) const
    {
        auto blocks_ptr = WorldMapGenerator::generate_flat_world(map_size);

        std::ofstream block_file(block_data_path);
        // block data header (map volume)
        map_volume = ::htonl(map_volume);
        block_file.write(reinterpret_cast<char*>(&map_volume), sizeof(map_volume));
        // raw block data
        block_file.write(blocks_ptr.get(), map_size.x * map_size.y * map_size.z);
    }


    void World::create_metadata_file(Coordinate3D map_size) const
    {
        WorldMetadata metadata;
        metadata.set_format_version(WORLD_METADATA_FORMAT_VERSION);
        
        metadata.set_width(map_size.x);
        metadata.set_height(map_size.y);
        metadata.set_length(map_size.z);
        metadata.set_volume(map_size.x * map_size.y * map_size.z);

        metadata.set_spawn_x(map_size.x / 2);
        metadata.set_spawn_y(map_size.y / 2);
        metadata.set_spawn_z(map_size.z / 2);
        metadata.set_spawn_yaw(0);
        metadata.set_spawn_pitch(0);

        metadata.set_created_at(util::current_timestmap());
            
        util::proto_message_to_json_file(metadata, world_metadata_path);
    }

    std::unique_ptr<BlockID[]> WorldMapGenerator::generate_flat_world(Coordinate3D map_size)
    {
        const auto plain_size = map_size.x * map_size.z;
        auto blocks = new BlockID[plain_size * map_size.y];

        auto num_of_dirt_block = plain_size * (map_size.y / 2);
        std::memset(blocks, BLOCK_DIRT, num_of_dirt_block);
        std::memset(blocks + num_of_dirt_block, BLOCK_GRASS, plain_size);
        std::memset(blocks, BLOCK_WATER, plain_size);

        return std::unique_ptr<BlockID[]>(blocks);
    }
}