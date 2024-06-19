#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include "game/block.h"
#include "game/player.h"
#include "proto/world_metadata.pb.h"
#include "net/connection_key.h"
#include "net/multicast_manager.h"
#include "win/file_mapping.h"
#include "util/common_util.h"
#include "util/lockfree_stack.h"

namespace net
{
    class ConnectionEnvironment;
}

namespace game
{
    constexpr const char* block_data_filename = "blocks.bin";
    constexpr const char* world_metadata_filename = "metadata.json";

    struct Coordinate3D
    {
        short x = 0;
        short y = 0;
        short z = 0;
    };

    class World : util::NonCopyable
    {
    public:
        World(net::ConnectionEnvironment&);

        game::Player* add_player(net::ConnectionKey, unsigned player_identity, game::PlayerType, const char* username, const char* password);

        void caching_compressed_block_data();

        void tick();

        bool load_filesystem_world(std::string_view);

    private:
        void load_metadata();

        void load_block_data();
        
        void create_new_world() const;

        void create_block_file(Coordinate3D map_size, unsigned long map_volume) const;

        void create_metadata_file(Coordinate3D map_size) const;

        net::ConnectionEnvironment& connection_env;
        net::MulticastManager multicast_manager;

        std::vector<std::unique_ptr<game::Player>> players;
        util::LockfreeStack<net::ConnectionKey> spawn_wait_players;

        WorldMetadata _metadata;

        win::FileMapping block_mapping;

        std::size_t last_saved_tick = 0;

        std::filesystem::path save_dir;
        std::filesystem::path block_data_path;
        std::filesystem::path world_metadata_path;

        static constexpr std::size_t block_data_cache_lifetime = 2 * 1000; // 2 seconds
        std::size_t block_data_cache_expired_at = 0;
    };

    class WorldMapGenerator
    {
    public:
        static std::unique_ptr<BlockID[]> generate_flat_world(Coordinate3D map_size);
    };
}