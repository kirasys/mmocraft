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

        void on_connection_delete(Connection&);

        // Check unresponsiveness connections (timeout) and delete these connection.
        void cleanup_expired_connection();

        void register_pending_connections();

        void flush_server_message();

        void flush_client_message();

        // Register player to the table by issuing unique player id.
        // * deferred packet thread invokes this method.
        std::pair<game::PlayerID, bool> register_player(unsigned);

    private:
        unsigned num_of_max_connections = 0;
        std::atomic<unsigned> num_of_connections{ 0 };

        util::LockfreeStack<win::ObjectPool<net::Connection>::Pointer> pending_connections;
        std::list<win::ObjectPool<net::Connection>::Pointer> connection_ptrs;
        std::unordered_set<Connection::Descriptor*> connection_table;
        
        std::unique_ptr<unsigned[]> player_lookup_table;
    };
}