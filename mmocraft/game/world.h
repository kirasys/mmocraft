#pragma once

#include <filesystem>
#include <memory>
#include <vector>
#include <shared_mutex>

#include "game/block.h"
#include "game/player.h"
#include "game/block_history.h"
#include "game/world_task.h"
#include "proto/generated/world_metadata.pb.h"
#include "net/connection_key.h"
#include "net/multicast_manager.h"
#include "win/file_mapping.h"
#include "util/common_util.h"

namespace database
{
    class DatabaseCore;
}

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
        
        bool is_already_exist_player(const char* username)
        {
            std::shared_lock lock(player_lookup_table_mutex);
            return player_lookup_table.find(username) != player_lookup_table.end();
        }

        void register_player(const char* username, net::ConnectionKey connection_key)
        {
            std::unique_lock lock(player_lookup_table_mutex);
            player_lookup_table[username] = connection_key;
        }

        void unregister_player(const char* username)
        {
            std::unique_lock lock(player_lookup_table_mutex);
            player_lookup_table.erase(username);
        }

        net::Connection* try_acquire_player_connection(const char* username);

        void unicast_to_world_player(const char* username, net::MessageType, const char* message);

        void broadcast_to_world_player(net::MessageType, const char* message);

        template <game::PlayerState T> 
        void send_to_specific_players(const std::byte* data, std::size_t data_size,
                        void(*successed)(game::Player*) = nullptr, void(*failed)(game::Player*) = nullptr)
        {
            std::vector<game::Player*> players;
            players.reserve(connection_env.size_of_max_connections());

            connection_env.select_players([](const game::Player* player)
                { return player->state() >= T; },
                players);

            send_to_players(players, data, data_size, successed, failed);
        }

        void process_level_wait_player(const std::vector<game::Player*>&);

        void spawn_player(const std::vector<game::Player*>&);

        void disconnect_player(const std::vector<game::Player*>&);

        void despawn_player(const std::vector<game::Player*>&);

        void sync_block(const std::vector<game::Player*>&, game::BlockHistory<>&);

        void sync_player_position();

        bool try_change_block(util::Coordinate3D, BlockID);

        void tick(io::RegisteredIO&);

        bool load_filesystem_world(std::string_view);

    private:
        void send_to_players(const std::vector<game::Player*>&, const std::byte*, std::size_t,
                void(*successed)(game::Player*) = nullptr, void(*failed)(game::Player*) = nullptr);

        void commit_block_changes(const std::byte* block_history_data, std::size_t);

        void load_metadata();

        void load_block_data();

        std::size_t coordinate_to_block_map_index(int x, int y, int z);

        net::ConnectionEnvironment& connection_env;
        database::DatabaseCore& database_core;

        net::MulticastManager multicast_manager;

        std::shared_mutex player_lookup_table_mutex;
        std::unordered_map<std::string, net::ConnectionKey> player_lookup_table;

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