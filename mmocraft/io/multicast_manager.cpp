#include "pch.h"
#include "multicast_manager.h"

#include <algorithm>

#include "config/constants.h"

namespace io
{
    std::shared_ptr<io::MulticastDataEntry> MulticastManager::create_data(io::multicast_tag_id::value tag, std::unique_ptr<std::byte[]>&& data, std::size_t data_size)
    {
        if (gc_timeouts[tag] < util::current_monotonic_tick())
            gc(tag);

        return std::make_shared<io::MulticastDataEntry>(std::move(data), data_size);
    }

    void MulticastManager::reset_data(io::multicast_tag_id::value tag)
    {
        active_data[tag] = nullptr;
    }

    int MulticastManager::gc(io::multicast_tag_id::value tag)
    {
        auto& data_queue = data_queues[tag];

        unsigned num_of_deleted = 0;
        while (not data_queue.empty()
                && active_data[tag] != &data_queue.front()
                && data_queue.front().is_safe_delete()) {
            data_queue.pop();
            num_of_deleted++;
        }

        gc_timeouts[tag] = util::current_monotonic_tick() + config::memory::multicast_data_gc_period_ms;
        return num_of_deleted;
    }
}