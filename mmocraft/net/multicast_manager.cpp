#include "pch.h"
#include "multicast_manager.h"

namespace net
{
    void MulticastManager::send(std::vector<net::ConnectionKey>& receivers, std::unique_ptr<std::byte>&& data, unsigned data_size)
    {
        if (util::current_monotonic_tick() > gc_timeout)
            gc();

        auto& event_data = event_data_pool.emplace(std::move(data), data_size, data_size, receivers.size());

        for (auto connection_key : receivers) {
            if (auto desc = connection_env.try_acquire_descriptor(connection_key)) {
                desc->multicast_send(&event_data);
            }
        }
    }

    void MulticastManager::gc()
    {
        unsigned num_of_deleted = 0;

        while (not event_data_pool.front().is_safe_delete()) {
            event_data_pool.pop();
            num_of_deleted++;
        }

        gc_timeout = util::current_monotonic_tick() + gc_intervals;
    }
}