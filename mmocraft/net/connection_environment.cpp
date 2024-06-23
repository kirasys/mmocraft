#include "pch.h"
#include "connection_environment.h"

#include <algorithm>
#include <cassert>

#include "util/time_util.h"

namespace net
{
    std::atomic<std::uint32_t> ConnectionEnvironment::connection_id_counter{ 0 };

    ConnectionEnvironment::ConnectionEnvironment(unsigned max_connections)
        : num_of_max_connections{ max_connections }
        , connection_table(max_connections)
    {
        
    }

    ConnectionID ConnectionEnvironment::get_unused_connection_id()
    {
        // find first unused slot.
        auto unused_slot = std::find_if(connection_table.begin(), connection_table.end(),
            [](const auto& entry) {
                return not entry.used.load(std::memory_order_relaxed);
            }
        );

        assert(unused_slot - connection_table.begin() < num_of_max_connections);
        return unsigned(unused_slot - connection_table.begin());
    }

    void ConnectionEnvironment::on_connection_create(ConnectionKey key, win::ObjectPool<net::Connection>::Pointer&& a_connection_ptr)
    {
        ++num_of_connections;

        auto& entry = connection_table[key.index()];
        entry.created_at = key.created_at();
        entry.connection = a_connection_ptr.get();
        entry.used.store(true, std::memory_order_release);
        entry.will_delete = false;

        entry.connection_life = std::move(a_connection_ptr);
    }

    void ConnectionEnvironment::on_connection_delete(ConnectionKey key)
    {
        auto& entry = connection_table[key.index()];
        entry.created_at = INVALID_TICK;
        entry.connection = nullptr;
        entry.used.store(false, std::memory_order_release);
    }

    void ConnectionEnvironment::on_connection_offline(ConnectionKey key)
    {
        auto& entry = connection_table[key.index()];
        entry.will_delete = true;
    }

    void ConnectionEnvironment::cleanup_expired_connection()
    {
        auto deleted_connection_count = std::count_if(connection_table.begin(), connection_table.end(),
            [](auto& entry) {
                if (not entry.used) return false;

                auto& desc = entry.connection->descriptor;

                if (desc.is_safe_delete()) {
                    entry.connection_life.reset();
                    return true;
                }

                if (desc.is_expired())
                    desc.disconnect();
                
                return false;
            }
        );

        num_of_connections -= unsigned(deleted_connection_count);
    }

    void ConnectionEnvironment::for_each_connection(void (*func) (net::Connection&))
    {
        for (auto& entry : connection_table) {
            if (not entry.will_delete)
                func(*entry.connection);
        }
    }

    void ConnectionEnvironment::for_each_descriptor(std::function<void(net::Connection::Descriptor&)> const& func)
    {
        for (auto& entry : connection_table) {
            if (not entry.will_delete)
                func(entry.connection->descriptor);
        }
    }

    void ConnectionEnvironment::for_each_player(std::function<void(net::Connection::Descriptor&, game::Player&)> const& func)
    {
        for (auto& entry : connection_table) {
            if (not entry.will_delete && entry.connection->descriptor.get_connected_player())
                func(entry.connection->descriptor, *entry.connection->descriptor.get_connected_player());
        }
    }

    void ConnectionEnvironment::select_players(bool(*filter)(const game::Player*), std::vector<game::Player*>& found)
    {
        for (std::size_t i = 0; i < connection_table.size(); i++) {
            auto& entry = connection_table[i];
            if (entry.will_delete)
                continue;

            auto player = entry.connection->descriptor.get_connected_player();
            if (player && filter(player))
                found.push_back(player);
        }

    }
}