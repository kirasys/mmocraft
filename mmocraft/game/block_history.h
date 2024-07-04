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
    constexpr std::size_t max_block_history_size = 1024 * 8;

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

    template <std::size_t MAX_HISTORY_SIZE = max_block_history_size>
    class BlockHistory
    {
    public:
        static constexpr std::size_t history_data_unit_size = 8;

        BlockHistory()
        {
            static_assert(sizeof(BlockHistoryRecord) == history_data_unit_size);
            reset();
        };

        ~BlockHistory()
        {
            if (auto data_ptr = _data.load(std::memory_order_relaxed))
                delete[] data_ptr;
        }

        std::size_t size() const
        {
            return std::min(MAX_HISTORY_SIZE, _size.load(std::memory_order_relaxed));
        }

        void reset(bool delete_data = true)
        {
            if (delete_data && _data) {
                delete[] _data.load(std::memory_order_relaxed);
            }

            _data.store(new std::byte[MAX_HISTORY_SIZE * history_data_unit_size], std::memory_order_relaxed);
            _size.store(0, std::memory_order_release);
        }

        static BlockHistoryRecord& get_record(std::byte* data, std::size_t index)
        {
            return *reinterpret_cast<BlockHistoryRecord*>(
                &data[index * history_data_unit_size]
            );
        }

        static const BlockHistoryRecord& get_record(const std::byte* data, std::size_t index)
        {
            return *reinterpret_cast<const BlockHistoryRecord*>(
                &data[index * history_data_unit_size]
                );
        }

        BlockHistoryRecord& get_record(std::size_t index)
        {
            return *reinterpret_cast<BlockHistoryRecord*>(
                &_data.load(std::memory_order_relaxed)[index * history_data_unit_size]
            );
        }

        bool add_record(util::Coordinate3D pos, game::BlockID block_id)
        {
            if (auto history_data_ptr = _data.load(std::memory_order_relaxed)) {
                auto index = _size.fetch_add(1, std::memory_order_relaxed);
                if (index >= MAX_HISTORY_SIZE) {
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

        std::size_t fetch_serialized_data(std::unique_ptr<std::byte[]>& history_data)
        {
            if (auto history_size = size()) {
                history_data.reset(_data.load(std::memory_order_relaxed));
                reset(false);
                return history_size * history_data_unit_size;
            }
            return 0;
        }


    private:
        std::atomic<std::size_t> _size{ 0 };
        std::atomic<std::byte*> _data{ nullptr };
    };
}