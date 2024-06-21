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

    constexpr std::size_t block_data_cache_lifetime = 2 * 1000; // 2 seconds
    constexpr std::size_t spawn_player_task_interval = 2 * 1000;    // 2 seconds.
    constexpr std::size_t sync_player_position_task_interval = 100; // 100 milliseconds.
    constexpr std::size_t ping_interval = 5 * 1000; // 5 seconds.

    class World final : util::NonCopyable, util::NonMovable
    {
    public:
        World(net::ConnectionEnvironment&);

        game::Player* add_player(net::ConnectionKey, unsigned player_identity, game::PlayerType, const char* username, const char* password);

        void caching_compressed_block_data();

        void spawn_player();

        void sync_player_position();

        void tick(io::IoCompletionPort&);

        bool load_filesystem_world(std::string_view);

    private:
        void load_metadata();

        void load_block_data();

        net::ConnectionEnvironment& connection_env;
        net::MulticastManager multicast_manager;

        std::vector<std::unique_ptr<game::Player>> players;
        util::LockfreeStack<game::Player*> spawn_wait_players;
        
        io::SimpleTask<game::World> spawn_player_task;
        io::SimpleTask<game::World> sync_player_position_task;

        WorldMetadata _metadata;

        win::FileMapping block_mapping;

        std::size_t last_save_map_at = 0;

        std::filesystem::path save_dir;
        std::filesystem::path block_data_path;
        std::filesystem::path metadata_path;

        std::size_t block_data_cache_expired_at = 0;
    };
}