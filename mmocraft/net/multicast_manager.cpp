#include "pch.h"
#include "multicast_manager.h"

#include <algorithm>

#include "net/connection.h"

namespace net
{
    void MulticastManager::set_multicast_data(MuticastTag tag, std::unique_ptr<std::byte[]>&& data, std::size_t data_size)
    {
        if (gc_timeout >= util::current_monotonic_tick())
            gc();

        auto& event_data = event_data_pool.emplace(
                 /*data = */ std::move(data),
            /*data_size = */ unsigned(data_size),
        /*data_capacity = */ unsigned(data_size));

        active_event_datas[tag] = &event_data;
    }

    void MulticastManager::reset_multicast_data(MuticastTag tag)
    {
        active_event_datas[tag] = nullptr;
    }

    bool MulticastManager::send(MuticastTag tag, net::Connection::Descriptor& connection_descriptor)
    {
        if (auto event_data = active_event_datas[tag]) {
            event_data->update_lifetime();
            return connection_descriptor.emit_multicast_send_event(event_data);
        }
        return false;
    }


    void MulticastManager::gc()
    {
        unsigned num_of_deleted = 0;

        while (not event_data_pool.empty() 
                && not is_active_event_data(event_data_pool.front())
                && event_data_pool.front().is_safe_delete()) {
            event_data_pool.pop();
            num_of_deleted++;
        }

        gc_timeout = util::current_monotonic_tick() + gc_intervals;
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