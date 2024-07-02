#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include "game/block.h"
#include "game/player.h"
#include "game/block_history.h"
#include "proto/world_metadata.pb.h"
#include "net/connection_key.h"
#include "net/multicast_manager.h"
#include "win/file_mapping.h"
#include "util/common_util.h"

namespace net
{
    class ConnectionEnvironment;
}

namespace game
{
    constexpr const char* block_data_filename = "blocks.bin";
    constexpr const char* world_metadata_filename = "metadata.json";

    constexpr std::size_t level_data_submission_interval     = 3 * 1000; // 3 seconds
    constexpr std::size_t spawn_player_task_interval         = 2 * 1000; // 2 seconds.
    constexpr std::size_t despawn_player_task_interval       = 2 * 1000; // 2 seconds.
    constexpr std::size_t sync_block_data_task_interval      = 200;      // 200 milliseconds.
    constexpr std::size_t sync_player_position_task_interval = 100;      // 100 milliseconds.
    constexpr std::size_t ping_interval                      = 5 * 1000; // 5 seconds.

    constexpr std::size_t max_block_history_size = 1024 * 8;

    class World final : util::NonCopyable, util::NonMovable
    {
    public:
        World(net::ConnectionEnvironment&);

        game::Player* add_player(net::ConnectionKey, unsigned player_identity, game::PlayerType, const char* username, const char* password);

        void multicast_to_world_player(net::MuticastTag, std::unique_ptr<std::byte[]>&&, std::size_t);

        void process_level_wait_player();

        void spawn_player();

        void despawn_player();

        void sync_block_data();

        void sync_player_position();

        bool try_change_block(util::Coordinate3D, BlockID);

        void tick(io::IoCompletionPort&);

        bool load_filesystem_world(std::string_view);

    private:
        void commit_block_changes(game::BlockHistory& block_history);

        void load_metadata();

        void load_block_data();

        std::size_t coordinate_to_block_map_index(int x, int y, int z);

        net::ConnectionEnvironment& connection_env;
        net::MulticastManager multicast_manager;

        std::vector<std::unique_ptr<game::Player>> players;

        std::vector<game::Player*> _level_wait_players;
        std::vector<game::Player*> _level_wait_player_queue;

        std::vector<game::Player*> _spawn_wait_players;
        std::vector<game::Player*> _spawn_wait_player_queue;

        std::vector<game::PlayerID> _despawn_wait_players;
        std::vector<game::PlayerID> _despawn_wait_player_queue;
        
        game::BlockHistory& get_inbound_block_history()
        {
            return block_histories[inbound_block_history_index.load(std::memory_order_relaxed)];
        }

        game::BlockHistory& get_outbound_block_history()
        {
            return block_histories[outbound_block_history_index];
        }

        std::atomic<unsigned> inbound_block_history_index = 0;
        unsigned outbound_block_history_index = 1;
        game::BlockHistory block_histories[2];

        io::SimpleTask<game::World> spawn_player_task;
        io::SimpleTask<game::World> despawn_player_task;
        io::SimpleTask<game::World> sync_block_task;
        io::SimpleTask<game::World> sync_player_position_task;

        WorldMetadata _metadata;

        win::FileMapping block_mapping;

        std::size_t last_save_map_at = 0;
        std::size_t last_level_data_submission_at = 0;

        std::filesystem::path save_dir;
        std::filesystem::path block_data_path;
        std::filesystem::path metadata_path;
    };
}