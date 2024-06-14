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
        , connection_table{ new ConnectionEntry[max_connections]() }
    {
        
    }

    unsigned ConnectionEnvironment::get_unused_slot()
    {
        // find first unused slot.
        auto unused_slot = std::find_if(connection_table.get(), connection_table.get() + num_of_max_connections,
            [](const auto& entry) {
                return not entry.used.load(std::memory_order_relaxed);
            }
        );

        assert(unused_slot - connection_table.get() < num_of_max_connections);
        return unsigned(unused_slot - connection_table.get());
    }

    void ConnectionEnvironment::on_connection_create(ConnectionKey key, win::ObjectPool<net::Connection>::Pointer&& a_connection_ptr)
    {
        ++num_of_connections;

        auto index = key.index();
        connection_table[index].created_at = key.created_at();
        connection_table[index].connection = a_connection_ptr.get();
        connection_table[index].used.store(true, std::memory_order_release);
        connection_table[index].will_delete = false;

        connection_table[index].connection_life = std::move(a_connection_ptr);
    }

    void ConnectionEnvironment::on_connection_delete(ConnectionKey key)
    {
        auto index = key.index();
        connection_table[index].created_at = 0;
        connection_table[index].connection = nullptr;
        connection_table[index].identity = 0;
        connection_table[index].used.store(false, std::memory_order_release);
    }

    void ConnectionEnvironment::on_connection_offline(ConnectionKey key)
    {
        auto index = key.index();
        connection_table[index].will_delete = true;
    }

    void ConnectionEnvironment::cleanup_expired_connection()
    {
        auto deleted_connection_count = std::count_if(connection_table.get(), connection_table.get() + num_of_max_connections,
            [](auto& entry) {
                if (not entry.used) return false;

                auto& desc = entry.connection->descriptor;

                if (desc.is_safe_delete()) {
                    entry.connection_life.reset();
                    return true;
                }

                if (desc.is_expired())
                    desc.set_offline();
                
                return false;
            }
        );

        num_of_connections -= unsigned(deleted_connection_count);
    }

    void ConnectionEnvironment::for_each_descriptor(void (*func) (net::Connection::Descriptor&))
    {
        std::for_each_n(connection_table.get(), num_of_max_connections,
            [func](auto& entry) { 
                if (not entry.will_delete) func(entry.connection->descriptor);
            }
        );
    }

    void ConnectionEnvironment::for_each_connection(void (*func) (net::Connection&))
    {
        std::for_each_n(connection_table.get(), num_of_max_connections,
            [func](auto& entry) {
                if (not entry.will_delete) func(*entry.connection);
            }
        );
    }

    bool ConnectionEnvironment::set_authentication_identity(ConnectionKey connection_key, unsigned identity)
    {
        auto is_dulicated_user_not_exist = std::all_of(connection_table.get(), connection_table.get() + num_of_max_connections,
            [identity](const auto& entry) {
                return !entry.identity || entry.identity != identity;
            }
        );

        connection_table[connection_key.index()].identity = identity;

        return is_dulicated_user_not_exist;
    }
}