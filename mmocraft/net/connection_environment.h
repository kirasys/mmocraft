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
    class ConnectionEnvironment : util::NonCopyable
    {
    public:
        ConnectionEnvironment();

        // used for testing purpose only.
        auto get_connection_pointers()
            -> std::list<win::ObjectPool<net::Connection>::Pointer>&
        {
            return connection_ptrs;
        }
        
        unsigned size_of_connections() const
        {
            return connection_counter.load();
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

        // Register player id to the lookup table.
        // * deferred packet thread invokes this method.
        bool register_player(game::PlayerID);

        // Fetch pending players and apply to the player lookup table.
        // * deferred packet thread invokes this method.
        void cleanup_deleted_player();

    private:
        std::atomic<unsigned> num_of_connections{ 0 };

        util::LockfreeStack<win::ObjectPool<net::Connection>::Pointer> pending_connections;
        std::list<win::ObjectPool<net::Connection>::Pointer> connection_ptrs;
        std::unordered_set<Connection::Descriptor*> connection_table;
        
        util::LockfreeStack<game::PlayerID> delete_pending_players;
        std::unordered_set<game::PlayerID> player_lookup_table;
    };
}