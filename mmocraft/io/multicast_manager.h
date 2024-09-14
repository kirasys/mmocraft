#pragma once

#include <queue>
#include <memory>

#include "win/registered_io.h"
#include "util/time_util.h"

namespace io
{
    enum MulticastTag
    {
        Level_Data,

        Spawn_Player,

        Despawn_Player,

        Sync_Block_Data,

        Sync_Player_Position,

        Chat_Message,

        MuticastTag_Count,
    };

    class MulticastDataEntry : util::NonCopyable
    {
    public:
        static constexpr std::size_t max_expiration_period = 3 * 60 * 1000; // 3 minutes.

        MulticastDataEntry(std::unique_ptr<std::byte[]>&& data, std::size_t data_size)
            : _data{ std::move(data) }
            , _data_size{ data_size }
            , _rio_buffer{ _data.get(), data_size }
        {
            update_lifetime();
        }

        void update_lifetime()
        {
            expire_at = max_expiration_period + util::current_monotonic_tick();
        }

        bool is_safe_delete() const
        {
            return (is_ref_count_mode && _ref_count.load() == 0) ||
                expire_at < util::current_monotonic_tick();
        }

        std::byte* data()
        {
            return _data.get();
        }

        std::size_t data_size() const
        {
            return _data_size;
        }

        RIO_BUFFERID buffer_id() const
        {
            return _rio_buffer.id();
        }

        void set_reference_count_mode(bool flag)
        {
            is_ref_count_mode = flag;
        }

        void increase_ref()
        {
            _ref_count.fetch_add(1);
        }

        void decrease_ref()
        {
            _ref_count.fetch_add(-1);
        }

    private:
        std::unique_ptr<std::byte[]> _data;
        std::size_t _data_size = 0;

        win::RioBufferPool _rio_buffer;

        bool is_ref_count_mode = false;
        std::atomic<int> _ref_count;
        std::size_t expire_at = 0;
    };

    class MulticastManager
    {
    public:
        MulticastManager()
        { }

        MulticastDataEntry& set_data(MulticastTag, std::unique_ptr<std::byte[]>&& data, std::size_t data_size);

        void reset_data(MulticastTag);

        void gc(MulticastTag);

    private:
        std::queue<MulticastDataEntry> data_queues[MulticastTag::MuticastTag_Count];

        MulticastDataEntry* active_data[MulticastTag::MuticastTag_Count];

        std::size_t gc_timeouts[MulticastTag::MuticastTag_Count];

        static constexpr std::size_t gc_period = 6 * 1000; // 6 seconds
    };
}