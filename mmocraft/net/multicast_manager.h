#pragma once

#include <queue>
#include <memory>

#include "io/io_event.h"
#include "net/connection_key.h"
#include "net/connection_environment.h"

#include "util/time_util.h"

namespace net
{
    enum MuticastTag
    {
        Level_Data,

        Spawn_Player,

        Despawn_Player,

        Sync_Block_Data,

        Sync_Player_Position,

        Chat_Message,

        MuticastTag_Count,
    };

    class MulticastManager
    {
    public:
        MulticastManager(net::ConnectionEnvironment& conn_env)
            : connection_env{ conn_env }
        {

        }

        void set_multicast_data(MuticastTag, std::unique_ptr<std::byte[]>&& data, std::size_t data_size);

        void reset_multicast_data(MuticastTag);

        bool send(MuticastTag, net::Connection::Descriptor&);

        void gc(MuticastTag);

    private:
        bool is_active_event_data(const io::IoSendEventSharedData&) const;

        net::ConnectionEnvironment& connection_env;

        std::queue<io::IoSendEventSharedData> event_data_queues[MuticastTag::MuticastTag_Count];

        io::IoSendEventSharedData* active_event_datas[MuticastTag::MuticastTag_Count];

        std::size_t gc_timeouts[MuticastTag::MuticastTag_Count];

        static constexpr std::size_t gc_intervals = 5 * 1000; // 5 seconds
    };
}