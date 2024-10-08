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

    void ConnectionEnvironment::on_connection_create(ConnectionKey key, std::unique_ptr<net::Connection>&& connection_ptr)
    {
        num_of_connections.fetch_add(1, std::memory_order_relaxed);

        auto& entry = connection_table[key.index()];
        entry.created_at = key.created_at();
        entry.connection = std::move(connection_ptr);
        entry.used.store(true, std::memory_order_release);
        entry.will_delete = false;
    }

    void ConnectionEnvironment::on_connection_delete(ConnectionKey key)
    {
        auto& entry = connection_table[key.index()];
        entry.created_at = INVALID_TICK;
        entry.used.store(false, std::memory_order_release);

        num_of_connections.fetch_sub(1, std::memory_order_relaxed);
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
                if (not entry.used.load(std::memory_order_relaxed)) return false;

                auto conn = entry.connection.get();

                if (conn->is_safe_delete()) {
                    entry.connection.reset();
                    return true;
                }

                if (conn->is_expired())
                    conn->disconnect();
                
                return false;
            }
        );
    }

    void ConnectionEnvironment::for_each_connection(void (*func) (net::Connection&))
    {
        for (auto& entry : connection_table) {
            if (not entry.will_delete)
                func(*entry.connection);
        }
    }

    void ConnectionEnvironment::for_each_player(std::function<void(net::Connection&, game::Player&)> const& func)
    {
        for (auto& entry : connection_table) {
            if (not entry.will_delete && entry.connection->associated_player())
                func(*entry.connection, *entry.connection->associated_player());
        }
    }

    void ConnectionEnvironment::select_players(bool(*filter)(const game::Player*), std::vector<game::Player*>& found)
    {
        for (std::size_t i = 0; i < connection_table.size(); i++) {
            auto& entry = connection_table[i];
            if (entry.will_delete)
                continue;

            auto player = entry.connection->associated_player();
            if (player && filter(player))
                found.push_back(player);
        }

    }
}