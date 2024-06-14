#pragma once

#include <cstdint>
#include <cstdlib>

namespace net
{
    class ConnectionEnvironment;

    // ConnectionKey class is used when need to safely access the connection.
    class ConnectionKey
    {
    public:
        ConnectionKey() : key{ 0 }
        { }

        ConnectionKey(unsigned index, std::size_t created_at) : key{ index | (created_at << 32) }
        { }

        inline unsigned index() const
        {
            return key & 0xFFFFFFFF;
        }

        inline unsigned created_at() const
        {
            return unsigned(key >> 32);
        }

    private:
        std::uint64_t key;
    };
}