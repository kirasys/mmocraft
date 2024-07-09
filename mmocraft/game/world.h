#pragma once

#include <filesystem>
#include <memory>
#include <vector>


#include "database/database_core.h"
#include "game/block.h"
#include "game/player.h"
#include "game/block_history.h"
#include "game/world_task.h"
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

    class World final : util::NonCopyable, util::NonMovable
    {
    public:
        World(net::ConnectionEnvironment&, database::DatabaseCore&);

        game::Player* add_player(net::ConnectionKey, unsigned player_identity, game::PlayerType, const char* username);

        void multicast_to_world_player(net::MuticastTag, std::unique_ptr<std::byte[]>&&, std::size_t);

        void broadcast_to_world_player(std::string_view message);

        void process_level_wait_player(const std::vector<game::Player*>&);

        void spawn_player(const std::vector<game::Player*>&);

        void disconnect_player(const std::vector<game::Player*>&);

        void despawn_player(const std::vector<game::Player*>&);

        void sync_block(const std::vector<game::Player*>&, game::BlockHistory<>&);

        void sync_player_position();

        bool try_change_block(util::Coordinate3D, BlockID);

        void tick(io::IoCompletionPort&);

        bool load_filesystem_world(std::string_view);

    private:
        void commit_block_changes(const std::byte* block_history_data, std::size_t);

        void load_metadata();

        void load_block_data();

        std::size_t coordinate_to_block_map_index(int x, int y, int z);

        net::ConnectionEnvironment& connection_env;
        database::DatabaseCore& database_core;

        net::MulticastManager multicast_manager;

        std::vector<std::unique_ptr<game::Player>> players;

        game::WorldPlayerTask spawn_player_task;
        game::WorldPlayerTask disconnect_player_task;
        game::BlockSyncTask sync_block_task;
        io::SimpleTask<game::World> sync_player_position_task;

        io::Task* world_tasks[4] = {
            &spawn_player_task,
            &disconnect_player_task,
            &sync_block_task,
            &sync_player_position_task,
        };

        WorldMetadata _metadata;

        win::FileMapping block_mapping;

        std::size_t last_save_map_at = 0;
        std::size_t last_level_data_submission_at = 0;

        std::filesystem::path save_dir;
        std::filesystem::path block_data_path;
        std::filesystem::path metadata_path;
    };
}