#pragma once

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <memory>

#include "net/packet.h"
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
    public:
        static constexpr std::size_t max_block_history_size = 1024;
        static constexpr std::size_t history_data_unit_size = 8;

        BlockHistory()
            : _data{ new std::byte[max_block_history_size * history_data_unit_size] }
        {
            static_assert(sizeof(BlockHistoryRecord) == history_data_unit_size);
        };

        ~BlockHistory()
        {
            if (auto data_ptr = _data.load(std::memory_order_relaxed))
                delete[] data_ptr;
        }

        std::size_t count() const
        {
            return std::min(max_block_history_size, _size.load(std::memory_order_relaxed));
        }

        void clear()
        {
            _size = 0;
        }

        static const BlockHistoryRecord& get_record(const std::byte* data, std::size_t index)
        {
            assert(index < max_block_history_size);

            return *reinterpret_cast<const BlockHistoryRecord*>(
                &data[index * history_data_unit_size]
                );
        }

        bool add_record(util::Coordinate3D pos, game::BlockID block_id)
        {
            if (auto history_data_ptr = _data.load(std::memory_order_relaxed)) {
                auto index = _size.fetch_add(1, std::memory_order_relaxed);
                if (index >= max_block_history_size) {
                    _size.fetch_sub(1, std::memory_order_relaxed);
                    return false;
                }

                auto& record = get_record(history_data_ptr, index);

                // Note: store coordinates as bin-endian in order to optimize serialization operation.
                record.packet_id = std::byte(net::PacketID::SetBlockServer);
                record.x = _byteswap_ushort(pos.x);
                record.y = _byteswap_ushort(pos.y);
                record.z = _byteswap_ushort(pos.z);
                record.block_id = std::byte(block_id);

                return true;
            }

            return false;
        }

        std::size_t serialize(std::unique_ptr<std::byte[]>& history_data) const
        {
            if (auto history_count = count()) {
                auto serialized_size = history_count * history_data_unit_size;
                history_data.reset(new std::byte[serialized_size]);
                std::memcpy(history_data.get(), _data.load(std::memory_order_relaxed), serialized_size);
                return serialized_size;
            }
            return 0;
        }

    private:

        static BlockHistoryRecord& get_record(std::byte* data, std::size_t index)
        {
            assert(index < max_block_history_size);

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