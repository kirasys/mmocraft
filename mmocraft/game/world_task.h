#pragma once

#include <vector>

#include "io/task.h"
#include "game/block.h"
#include "game/block_history.h"
#include "util/math.h"

namespace game
{
    class World;

    class WorldPlayerTask : public io::Task
    {
    public:
        using handler_type = void (game::World::* const)(const std::vector<game::Player*>&);

        WorldPlayerTask(handler_type handler, game::World* world_inst, std::size_t interval_ms = 0)
            : io::Task{ interval_ms }
            , _handler{ handler }
            , _world_inst{ world_inst }
        { }

        virtual bool ready() const override
        {
            return io::Task::ready() && not _players_queue.empty();
        }

        virtual void before_scheduling() override
        {
            set_state(State::processing);
            _players_queue.swap(_players_target);
        }

        void push(game::Player* task_data)
        {
            _players_queue.push_back(task_data);
        }

        virtual void on_event_complete(void* completion_key, DWORD transferred_bytes) override
        {
            std::invoke(_handler, _world_inst, _players_target);

            _players_target.clear();
            set_state(State::unused);
        }

    private:
        handler_type _handler;
        game::World* _world_inst = nullptr;

        std::vector<game::Player*> _players_target;
        std::vector<game::Player*> _players_queue;
    };

    class BlockSyncTask : public io::Task
    {
    public:
        using handler_type = void (game::World::* const)(const std::vector<game::Player*>&, util::byte_view);

        BlockSyncTask(handler_type handler, game::World* world, std::size_t interval_ms = 0)
            : io::Task{ interval_ms }
            , _handler{ handler }
            , _world{ world }
        { }

        virtual bool ready() const override
        {
            return io::Task::ready() && (block_history.has_live_data() || not _level_wait_player_queue.empty());
        }

        virtual void before_scheduling() override
        {
            _level_wait_player_queue.swap(_level_wait_players);
            block_history.snapshot();

            set_state(State::processing);
        }

        void push(game::Player* task_data)
        {
            _level_wait_player_queue.push_back(task_data);
        }

        bool push(util::Coordinate3D pos, game::BlockID block_id)
        {
            return block_history.add_record(pos, block_id);
        }

        virtual void on_event_complete(void* completion_key, DWORD transferred_bytes) override
        {
            std::invoke(_handler, _world, _level_wait_players, block_history.get_snapshot_data());

            _level_wait_players.clear();
            block_history.clear_snapshot();
            set_state(State::unused);
        }

    private:
        handler_type _handler;
        game::World* _world = nullptr;

        game::BlockHistory block_history;

        std::vector<game::Player*> _level_wait_players;
        std::vector<game::Player*> _level_wait_player_queue;
    };
}