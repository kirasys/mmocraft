#pragma once
#include <list>

#include "io/io_event_pool.h"
#include "net/connection.h"

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
        
        void append_connection(win::ObjectPool<net::Connection>::Pointer&& a_connection_ptr)
        {
            connection_ptrs.emplace_back(std::move(a_connection_ptr));
        }

        std::size_t size_of_connections() const
        {
            return connection_ptrs.size();
        }

        void cleanup_expired_connection();

    private:
        std::list<win::ObjectPool<net::Connection>::Pointer> connection_ptrs;
    };
}