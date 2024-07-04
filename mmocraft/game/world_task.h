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

        WorldPlayerTask(handler_type handler, game::World* world, std::size_t interval_ms = 0)
            : io::Task{ interval_ms }
            , _handler{ handler }
            , _world{ world }
        { }

        virtual bool ready() const override
        {
            return io::Task::ready() && not _task_data_queue.empty();
        }

        virtual void before_scheduling() override
        {
            set_state(State::Processing);
            _task_data_queue.swap(_task_datas);
        }

        void push(game::Player* task_data)
        {
            _task_data_queue.push_back(task_data);
        }

        virtual void invoke_handler(ULONG_PTR task_handler_inst) override
        {
            try {
                std::invoke(_handler, _world, _task_datas);
            }
            catch (...) {
                CONSOLE_LOG(error) << "Unexpected error occured at simple task handler";
            }

            _task_datas.clear();
            set_state(State::Unused);
        }

    private:
        handler_type _handler;
        game::World* _world = nullptr;

        std::vector<game::Player*> _task_datas;
        std::vector<game::Player*> _task_data_queue;
    };

    class BlockSyncTask : public io::Task
    {
    public:
        using handler_type = void (game::World::* const)(const std::vector<game::Player*>&, game::BlockHistory<>&);

        BlockSyncTask(handler_type handler, game::World* world, std::size_t interval_ms = 0)
            : io::Task{ interval_ms }
            , _handler{ handler }
            , _world{ world }
        { }

        virtual bool ready() const override
        {
            return io::Task::ready() && (block_histories[inbound_block_history_index].size() || not _level_wait_player_queue.empty());
        }

        virtual void before_scheduling() override
        {
            _level_wait_player_queue.swap(_level_wait_players);
            outbound_block_history_index = inbound_block_history_index.exchange(outbound_block_history_index, std::memory_order_relaxed);
            set_state(State::Processing);
        }

        void push(game::Player* task_data)
        {
            _level_wait_player_queue.push_back(task_data);
        }

        bool push(util::Coordinate3D pos, game::BlockID block_id)
        {
            return block_histories[inbound_block_history_index].add_record(pos, block_id);
        }

        virtual void invoke_handler(ULONG_PTR task_handler_inst) override
        {
            try {
                std::invoke(_handler, _world, _level_wait_players, block_histories[outbound_block_history_index]);
            }
            catch (...) {
                CONSOLE_LOG(error) << "Unexpected error occured at simple task handler";
            }

            _level_wait_players.clear();
            set_state(State::Unused);
        }

    private:
        handler_type _handler;
        game::World* _world = nullptr;

        std::atomic<unsigned> inbound_block_history_index = 0;
        unsigned outbound_block_history_index = 1;
        game::BlockHistory<> block_histories[2];

        std::vector<game::Player*> _level_wait_players;
        std::vector<game::Player*> _level_wait_player_queue;
    };
}