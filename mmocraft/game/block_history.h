#pragma once

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <memory>

#include "game/block.h"
#include "util/math.h"
#include "util/double_buffering.h"
#include "config/constants.h"

namespace game
{
    // Note: Must be packed in order to optimize serialization operation.
    #pragma pack(push, 1)
    struct BlockHistoryRecord
    {
        std::byte packet_id;
        std::uint16_t x;
        std::uint16_t y;
        std::uint16_t z;
        std::byte block_id;
    };
    #pragma pack(pop)

    class BlockHistory : public util::DoubleBuffering
    {
    public:
        
        static constexpr std::size_t history_data_unit_size = sizeof(BlockHistoryRecord);

        bool add_record(util::Coordinate3D pos, game::BlockID block_id);

        static const BlockHistoryRecord& get_record(const std::byte* history_data, std::size_t index)
        {
            assert(index < config::memory::block_history_max_count);

            return *reinterpret_cast<const BlockHistoryRecord*>(
                &history_data[index * history_data_unit_size]
            );
        }

        static std::size_t size(std::size_t history_data_size)
        {
            return history_data_size / history_data_unit_size;
        }
    };
}