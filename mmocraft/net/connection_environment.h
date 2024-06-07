#pragma once
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
        
        void append_connection(win::ObjectPool<net::Connection>::Pointer&&);

        std::size_t size_of_connections() const
        {
            return connection_ptrs.size();
        }

        void cleanup_expired_connection();

        void activate_pending_connections();

        void flush_server_message();

        void flush_client_message();

    private:
        std::list<win::ObjectPool<net::Connection>::Pointer> connection_ptrs;

        util::LockfreeStack<Connection::Descriptor*> pending_connection;

        std::unordered_set<Connection::Descriptor*> online_connection_table;
    };
}