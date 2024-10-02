#include "pch.h"
#include "block_history.h"

namespace game
{
    bool BlockHistory::add_record(util::Coordinate3D pos, game::BlockID block_id)
    {
        if (auto history_data_ptr = _data.load(std::memory_order_relaxed)) {
            auto index = _size.fetch_add(1, std::memory_order_relaxed);
            if (index >= config::memory::block_history_max_count) {
                _size.fetch_sub(1, std::memory_order_relaxed);
                return false;
            }

            auto& record = get_record(history_data_ptr, index);

            // Note: store coordinates as bin-endian in order to optimize serialization operation.
            record.packet_id = std::byte(net::packet_type_id::set_block_server);
            record.x = _byteswap_ushort(pos.x);
            record.y = _byteswap_ushort(pos.y);
            record.z = _byteswap_ushort(pos.z);
            record.block_id = std::byte(block_id);

            return true;
        }

        return false;
    }

    std::size_t BlockHistory::serialize(std::unique_ptr<std::byte[]>& history_data) const
    {
        if (auto history_count = count()) {
            auto serialized_size = history_count * history_data_unit_size;
            history_data.reset(new std::byte[serialized_size]);
            std::memcpy(history_data.get(), _data.load(std::memory_order_relaxed), serialized_size);
            return serialized_size;
        }
        return 0;
    }
}