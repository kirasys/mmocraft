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
        bool online = false;

        // it's the same as player identity number, so we can get it by dereferencing the connection.
        // but, dereferencing offlined conenctions should be avoid. use this identity instead.
        unsigned identity = 0;      // (always greater than 0)
        net::Connection* connection;
    };

    class ConnectionEnvironment : util::NonCopyable
    {
    public:
        ConnectionEnvironment(unsigned);

        // used for testing purpose only.
        auto get_connection_pointers()
            -> std::list<win::ObjectPool<net::Connection>::Pointer>&
        {
            return connection_ptrs;
        }
        
        unsigned size_of_connections() const
        {
            return num_of_connections.load();
        }

        // Append new allocated conneciton resource to manage life-cycle.
        // * the accept I/O thread invokes this method, so append to the lock-free stack(pending_connections) first.
        void append_connection(win::ObjectPool<net::Connection>::Pointer&&);

        void on_connection_delete(unsigned);

        void on_connection_offline(unsigned);

        // Check unresponsiveness connections (timeout) and delete these connection.
        void cleanup_expired_connection();

        void flush_server_message();

        void flush_client_message();

        // Set identity if there are no already logged in users using same identity.
        // * deferred packet thread invokes this method.
        bool set_authentication_key(unsigned, unsigned);

    private:
        unsigned num_of_max_connections = 0;
        // number of active connections. it is used to limit accepting new clients.
        std::atomic<unsigned> num_of_connections{ 0 };

        unsigned get_unused_table_index();
        std::list<win::ObjectPool<net::Connection>::Pointer> connection_ptrs;
        std::unique_ptr<ConnectionEntry[]> connection_table;
    };
}