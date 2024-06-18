#pragma once
#include <atomic>
#include <bitset>
#include <functional>
#include <list>
#include <vector>
#include <unordered_set>

#include "io/io_event_pool.h"
#include "game/player.h"
#include "net/connection.h"
#include "net/connection_key.h"
#include "util/lockfree_stack.h"

#define INVALID_TICK 0xDEADBEEF

namespace net
{
    struct ConnectionEntry
    {
        std::atomic<bool> used{ false };
        bool will_delete = true;

        net::Connection* connection = nullptr;
        win::ObjectPool<net::Connection>::Pointer connection_life;

        std::uint32_t created_at = INVALID_TICK;
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

        std::uint32_t is_expired(ConnectionKey key) const
        {
            return connection_table[key.index()].will_delete 
                || key.created_at() != connection_table[key.index()].created_at;
        }

        net::Connection* try_acquire_connection(ConnectionKey key) const
        {
            return is_expired(key) ? nullptr : connection_table[key.index()].connection;
        }

        net::Connection::Descriptor* try_acquire_descriptor(ConnectionKey key) const
        {
            return is_expired(key) ? nullptr : &connection_table[key.index()].connection->descriptor;
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

        // Invoked after connection constructor finished.
        // Register new created conneciton to the table and owns connection resource.
        // * the accept I/O thread invokes this method.
        void on_connection_create(ConnectionKey, win::ObjectPool<net::Connection>::Pointer&&);

        // Invoked at the connection destructor.
        // * the accept I/O thread invokes this method.
        void on_connection_delete(ConnectionKey);

        // Invoked when connection goes offline. (thread-safe)
        void on_connection_offline(ConnectionKey);

        // Check unresponsiveness connections (timeout) and delete these connection.
        // * the accept I/O thread invokes this method.
        void cleanup_expired_connection();

        void for_each_connection(void (*func) (net::Connection&));

        void for_each_descriptor(std::function<void(net::Connection::Descriptor&)> const&);

        void for_each_player(std::function<void(net::Connection::Descriptor&, game::Player&)> const&);

        void select_players(unsigned n, std::bitset<1024>* [], unsigned [], bool(*[])(net::ConnectionKey));

        void poll_players(std::vector<std::unique_ptr<game::Player>>&,
                            unsigned filter_count,
                            bool(*[])(const game::Player*),
                            std::vector<unsigned>* matched_index_sets[]);

    private:
        static std::atomic<std::uint32_t> connection_id_counter;
        
        unsigned num_of_max_connections = 0;
        // number of active connections. it is used to limit accepting new clients.
        std::atomic<unsigned> num_of_connections{ 0 };

        std::vector<ConnectionEntry> connection_table;
    };
}