#pragma once

#include <atomic>
#include <memory>

#include "game/block.h"
#include "util/math.h"

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
        static constexpr std::size_t history_data_unit_size = 8;

    public:
        BlockHistory()
        {
            static_assert(sizeof(BlockHistoryRecord) == history_data_unit_size);
        };

        ~BlockHistory();

        std::size_t size() const
        {
            return std::min(_max_size, _size.load(std::memory_order_relaxed));
        }

        void initialize(std::size_t max_size);

        void reset();

        BlockHistoryRecord& get_record(std::byte* data, std::size_t index)
        {
            return *reinterpret_cast<BlockHistoryRecord*>(
                &data[index * history_data_unit_size]
            );
        }

        BlockHistoryRecord& get_record(std::size_t index)
        {
            return *reinterpret_cast<BlockHistoryRecord*>(
                &_data.load(std::memory_order_relaxed)[index * history_data_unit_size]
            );
        }

        bool add_record(util::Coordinate3D, game::BlockID);

        std::size_t fetch_serialized_data(std::unique_ptr<std::byte[]>& history_data);


    private:
        std::size_t _max_size;
        std::atomic<std::size_t> _size{ 0 };
       
        bool data_ownership_moved = false;
        std::atomic<std::byte*> _data{ nullptr };
    };
}