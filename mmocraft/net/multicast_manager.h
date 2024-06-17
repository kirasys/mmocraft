#pragma once

#include <queue>
#include <memory>

#include "io/io_event.h"
#include "net/connection_key.h"
#include "net/connection_environment.h"

#include "util/time_util.h"

namespace net
{
    class MulticastManager
    {
    public:
        MulticastManager(net::ConnectionEnvironment& conn_env)
            : connection_env{ conn_env }
            , gc_timeout{ util::current_monotonic_tick() + gc_intervals }
        {

        }

        void send(std::vector<net::ConnectionKey>& receivers, std::unique_ptr<std::byte>&& data, unsigned data_size);

        void gc();

    private:
        net::ConnectionEnvironment& connection_env;

        std::queue<io::IoSendEventSharedData> event_data_pool;

        static constexpr std::size_t gc_intervals = 5 * 1000; // 5 seconds
        std::size_t gc_timeout = 0;
    };
}