#pragma once
#include <atomic>
#include <list>
#include <unordered_map>
#include <unordered_set>

#include "io/io_event_pool.h"
#include "net/connection.h"
#include "util/lockfree_stack.h"

namespace net
{
    class ConnectionEnvironment
    {
    public:
        auto get_connection_pointers()
            -> std::list<win::ObjectPool<net::Connection>::Pointer>&
        {
            return connection_ptrs;
        }
        
        int size_of_connections() const
        {
            return connection_counter.load();
        }

        // Append new allocated conneciton resource to manage life-cycle.
        // because this method invoked at the accept I/O thread, append to the lock-free stack(pending_connections) first.
        void append_connection(win::ObjectPool<net::Connection>::Pointer&&);

        // Check unresponsiveness connections (timeout) and delete these connection.
        void cleanup_expired_connection();

        void register_pending_connections();

        void flush_server_message();

        void flush_client_message();

    private:
        std::atomic<int> connection_counter{ 0 };

        util::LockfreeStack<win::ObjectPool<net::Connection>::Pointer> pending_connections;
        
        std::list<win::ObjectPool<net::Connection>::Pointer> connection_ptrs;

        std::unordered_set<Connection::Descriptor*> connection_table;
    };
}