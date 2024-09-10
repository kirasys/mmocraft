#include "pch.h"
#include "multicast_manager.h"

#include <algorithm>

#include "net/connection.h"

namespace net
{
    void MulticastManager::set_multicast_data(MuticastTag tag, std::unique_ptr<std::byte[]>&& data, std::size_t data_size)
    {
        if (gc_timeouts[tag] < util::current_monotonic_tick())
            gc(tag);

        auto& event_data = event_data_queues[tag].emplace(
                 /*data = */ std::move(data),
            /*data_size = */ unsigned(data_size),
        /*data_capacity = */ unsigned(data_size));

        active_event_datas[tag] = &event_data;
    }

    void MulticastManager::reset_multicast_data(MuticastTag tag)
    {
        active_event_datas[tag] = nullptr;
    }

    bool MulticastManager::send(MuticastTag tag, net::Connection* conn)
    {
        if (auto event_data = active_event_datas[tag]) {
            event_data->update_lifetime();
            //return conn ? conn->io()->emit_multicast_send_event(event_data) : false;
        }
        return false;
    }

    void MulticastManager::gc(MuticastTag tag)
    {
        auto& event_data_queue = event_data_queues[tag];

        unsigned num_of_deleted = 0;
        while (not event_data_queue.empty()
                && not is_active_event_data(event_data_queue.front())
                && event_data_queue.front().is_safe_delete()) {
            event_data_queue.pop();
            num_of_deleted++;
        }

        gc_timeouts[tag] = util::current_monotonic_tick() + gc_intervals;
    }

    bool MulticastManager::is_active_event_data(const io::IoSendEventSharedData& event_data) const
    {
        for (unsigned i = 0; i < MuticastTag::MuticastTag_Count; i++) {
            if (active_event_datas[i] == &event_data)
                return true;
        }
        return false;
    }
}