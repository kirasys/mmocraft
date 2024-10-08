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

    class BlockHistory : public util::DoubleBuffering<config::memory::block_history_capacity>
    {
    public:
        
        static constexpr std::size_t history_data_unit_size = sizeof(BlockHistoryRecord);

        bool add_record(util::Coordinate3D pos, game::BlockID block_id);

        const BlockHistoryRecord& get_record(std::size_t index) const
        {
            assert(index < config::memory::block_history_capacity / history_data_unit_size);

            return *reinterpret_cast<const BlockHistoryRecord*>(
                &get_snapshot_data().data()[index * history_data_unit_size]
            );
        }

        std::size_t size() const
        {
            return get_snapshot_data().size() / history_data_unit_size;
        }
    };

    class CommonChatHistory : public util::DoubleBuffering<config::memory::common_chat_history_capacity>
    {
    public:

        bool add_record(util::byte_view packet_data)
        {
            return input_buffer().push(packet_data.data(), packet_data.size());
        }

    private:

    };
}