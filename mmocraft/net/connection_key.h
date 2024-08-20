#pragma once

#include <cstdint>
#include <cstdlib>

namespace net
{
    class ConnectionEnvironment;

    using ConnectionID = unsigned;

    // ConnectionKey class is used when need to safely access the connection.
    class ConnectionKey
    {
    public:
        ConnectionKey() : key{ 0 }
        { }

        ConnectionKey(std::uint64_t raw_key) : key{ raw_key }
        { }

        ConnectionKey(ConnectionID index, std::size_t created_at) : key{ index | (created_at << 32) }
        { }

        inline ConnectionID index() const
        {
            return key & 0xFFFFFFFF;
        }

        inline unsigned created_at() const
        {
            return unsigned(key >> 32);
        }

        bool operator==(ConnectionKey rhs) const
        {
            return key == rhs.key;
        }

        auto raw() const
        {
            return key;
        }

    private:
        std::uint64_t key;
    };
}