#include "pch.h"
#include "block_history.h"

#include "net/packet.h"

namespace game
{
    bool BlockHistory::add_record(util::Coordinate3D pos, game::BlockID block_id)
    {
        BlockHistoryRecord record{
            .packet_id = std::byte(net::packet_type_id::set_block_server),
            .x = _byteswap_ushort(pos.x),
            .y = _byteswap_ushort(pos.y),
            .z = _byteswap_ushort(pos.z),
            .block_id = std::byte(block_id)
        };

        auto& buffer = input_buffer();
        return buffer.push(reinterpret_cast<const std::byte*>(&record), history_data_unit_size);
    }
}