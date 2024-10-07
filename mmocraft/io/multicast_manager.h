#pragma once

#include <queue>
#include <memory>

#include "win/registered_io.h"
#include "util/time_util.h"

namespace io
{
    namespace multicast_tag_id
    {
        enum value
        {
            level_data,

            spawn_player,

            despawn_player,

            sync_block,

            sync_Player_position,

            common_chat_message,

            count,
        };
    }
    

    class MulticastDataEntry : util::NonCopyable
    {
    public:
        static constexpr std::size_t max_expiration_period = 3 * 60 * 1000; // 3 minutes.

        MulticastDataEntry(std::unique_ptr<std::byte[]>&& data, std::size_t data_size)
            : _data{ std::move(data) }
            , _data_size{ data_size }
            , registered_buffer{ _data.get(), data_size }
        {
            update_lifetime();
        }

        void update_lifetime()
        {
            expire_at = max_expiration_period + util::current_monotonic_tick();
        }

        bool is_safe_delete() const
        {
            return (ref_count_mode && ref_count.load() == 0) ||
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

        RIO_BUFFERID registered_buffer_id() const
        {
            return registered_buffer.id();
        }

        void set_reference_count_mode(bool flag)
        {
            ref_count_mode = flag;
        }

        void increase_ref()
        {
            ref_count.fetch_add(1);
        }

        void decrease_ref()
        {
            ref_count.fetch_add(-1);
        }

    private:
        std::unique_ptr<std::byte[]> _data;
        std::size_t _data_size = 0;

        win::RioBufferPool registered_buffer;

        bool ref_count_mode = false;
        std::atomic<int> ref_count;
        std::size_t expire_at = 0;
    };

    class MulticastManager
    {
    public:
        MulticastManager()
        { }

        MulticastDataEntry& set_data(io::multicast_tag_id::value, std::unique_ptr<std::byte[]>&& data, std::size_t data_size);

        void reset_data(io::multicast_tag_id::value);

        void gc(io::multicast_tag_id::value);

    private:
        std::queue<MulticastDataEntry> data_queues[io::multicast_tag_id::count];

        MulticastDataEntry* active_data[io::multicast_tag_id::count];

        std::size_t gc_timeouts[io::multicast_tag_id::count];
    };
}