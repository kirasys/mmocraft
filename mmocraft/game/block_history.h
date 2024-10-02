#pragma once

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <memory>

#include "net/packet.h"
#include "game/block.h"
#include "util/math.h"
#include "config/constants.h"

namespace game
{
    // Note: Must be packed in order to optimize serialization operation.
    #pragma pack(push, 1)
    struct BlockHistoryRecord
    {
        std::byte packet_id;
        std::int16_t x;
        std::int16_t y;
        std::int16_t z;
        std::byte block_id;
    };
    #pragma pack(pop)

    class BlockHistory
    {
    public:
        static constexpr std::size_t history_data_unit_size = sizeof(BlockHistoryRecord);

        BlockHistory()
            : _data{ new std::byte[config::memory::block_history_max_count * history_data_unit_size] }
        { }

        ~BlockHistory()
        {
            if (auto data_ptr = _data.load(std::memory_order_relaxed))
                delete[] data_ptr;
        }

        std::size_t count() const
        {
            return std::min(config::memory::block_history_max_count, _size.load(std::memory_order_relaxed));
        }

        void clear()
        {
            _size = 0;
        }

        static const BlockHistoryRecord& get_record(const std::byte* data, std::size_t index)
        {
            assert(index < config::memory::block_history_max_count);

            return *reinterpret_cast<const BlockHistoryRecord*>(
                &data[index * history_data_unit_size]
                );
        }

        bool add_record(util::Coordinate3D pos, game::BlockID block_id);

        std::size_t serialize(std::unique_ptr<std::byte[]>& history_data) const;

    private:

        static BlockHistoryRecord& get_record(std::byte* data, std::size_t index)
        {
            assert(index < config::memory::block_history_max_count);

            return *reinterpret_cast<BlockHistoryRecord*>(
                &data[index * history_data_unit_size]
                );
        }

        std::atomic<std::size_t> _size{ 0 };
        std::atomic<std::byte*> _data{ nullptr };
    };

    class BlockHistoryBuffer
    {
    public:

        bool add_record(util::Coordinate3D pos, game::BlockID block_id)
        {
            return _block_history[inbound_block_history_index].add_record(pos, block_id);
        }

        std::size_t count() const
        {
            return _block_history[inbound_block_history_index].count();
        }

        void flush()
        {
            inbound_block_history_index ^= 1;
        }

        const game::BlockHistory& get() const
        {
            return _block_history[inbound_block_history_index ^ 1];
        }

        void clear()
        {
            _block_history[inbound_block_history_index ^ 1].clear();
        }

    private:
        std::atomic<int> inbound_block_history_index{ 0 };
        game::BlockHistory _block_history[2];
    };
}