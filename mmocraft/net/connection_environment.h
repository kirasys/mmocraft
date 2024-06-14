#pragma once
#include <atomic>
#include <list>
#include <vector>
#include <unordered_set>

#include "io/io_event_pool.h"
#include "net/connection.h"
#include "util/lockfree_stack.h"

namespace net
{
    struct ConnectionEntry
    {
        std::atomic<bool> used{ false };
        bool will_delete = true;

        // it's the same as player identity number, so we can get it by dereferencing the connection.
        // but, dereferencing offlined conenctions should be avoid. use this identity instead.
        // it is used to verify that no same (identity) users already logged in.
        unsigned identity = 0;      // (always greater than 0)

        net::Connection* connection;
        win::ObjectPool<net::Connection>::Pointer connection_life;

        std::uint32_t created_at = 0;
    };

    class ConnectionEnvironment : util::NonCopyable
    {
    public:
        ConnectionEnvironment(unsigned);

        // used for testing purpose only.
        auto get_connection(int index)
        {
            return connection_table[index].connection;
        }
        
        unsigned size_of_connections() const
        {
            return num_of_connections.load();
        }

        unsigned size_of_max_connections() const
        {
            return num_of_max_connections;
        }

        unsigned get_unused_slot();

        // Append new allocated conneciton resource to manage life-cycle.
        // * the accept I/O thread invokes this method, so append to the lock-free stack(pending_connections) first.
        void add_connection(ConnectionKey, win::ObjectPool<net::Connection>::Pointer&&);

        void on_connection_delete(ConnectionKey);

        void on_connection_offline(ConnectionKey);

        // Check unresponsiveness connections (timeout) and delete these connection.
        void cleanup_expired_connection();

        void for_each_descriptor(void (*func) (net::Connection::Descriptor&));

        void for_each_connection(void (*func) (net::Connection&));

        // Set identity if there are no already logged in users using same identity.
        // * deferred packet thread invokes this method.
        bool set_authentication_identity(ConnectionKey, unsigned);

    private:
        static std::atomic<std::uint32_t> connection_id_counter;
        
        unsigned num_of_max_connections = 0;
        // number of active connections. it is used to limit accepting new clients.
        std::atomic<unsigned> num_of_connections{ 0 };

        std::unique_ptr<ConnectionEntry[]> connection_table;
    };
}