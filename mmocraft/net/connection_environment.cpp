#include "pch.h"
#include "connection_environment.h"

namespace net
{
    void ConnectionEnvironment::cleanup_expired_connection()
    {
        for (auto it = connection_ptrs.begin(); it != connection_ptrs.end();) {
            auto& connection = *(*it).get();

            if (connection.descriptor.is_safe_delete()) {
                it = connection_ptrs.erase(it);
                continue;
            }

            if (connection.descriptor.is_expired())
                connection.descriptor.set_offline();

            ++it;
        }
    }
}