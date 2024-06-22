#include "pch.h"
#include "block_history.h"

#include <cassert>
#include <cstdlib>
#include "net/packet.h"

namespace game
{
    BlockHistory::~BlockHistory()
    {
        if (auto data_ptr = _history_data.load(std::memory_order_relaxed))
            delete[] data_ptr;
    }

    void BlockHistory::initialize(std::size_t max_size)
    {
        if (not _history_data) {
            _max_size = max_size;
            reset();
        }
    }

    void BlockHistory::reset()
    {
        if (not data_ownership_moved && _history_data) {
            delete[] _history_data.load(std::memory_order_relaxed);
        }

        data_ownership_moved = false;
        _history_data.store(new std::byte[_max_size * history_data_unit_size], std::memory_order_relaxed);
        _size.store(0, std::memory_order_release);
    }

    bool BlockHistory::add_record(util::Coordinate3D pos, game::BlockID block_id)
    {
        if (auto history_data_ptr = _history_data.load(std::memory_order_relaxed)) {
            auto index = _size.fetch_add(1, std::memory_order_relaxed);
            if (index >= _max_size) {
                _size.fetch_sub(1, std::memory_order_relaxed);
                return false;
            }

            auto record = reinterpret_cast<BlockHistoryRecord*>(&history_data_ptr[index * history_data_unit_size]);

            // Note: store coordinates as bin-endian in order to optimize serialization operation.
            record->packet_id = std::byte(net::PacketID::SetBlockServer);
            record->x = _byteswap_ushort(pos.x);
            record->y = _byteswap_ushort(pos.y);
            record->z = _byteswap_ushort(pos.z);
            record->block_id = std::byte(block_id);

            return true;
        }

        return false;
    }

    std::size_t BlockHistory::fetch_serialized_data(std::unique_ptr<std::byte[]>& history_data)
    {
        if (size()) {
            data_ownership_moved = true;
            history_data.reset(_history_data.load(std::memory_order_relaxed));
        }
        return size() * history_data_unit_size;
    }
}