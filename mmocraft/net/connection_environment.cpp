#include "pch.h"
#include "connection_environment.h"

namespace net
{
    void ConnectionEnvironment::append_connection(win::ObjectPool<net::Connection>::Pointer&& a_connection_ptr)
    {
        pending_connection.push(&(a_connection_ptr.get()->descriptor));
        connection_ptrs.emplace_back(std::move(a_connection_ptr));
    }

    void ConnectionEnvironment::cleanup_expired_connection()
    {
        for (auto it = connection_ptrs.begin(); it != connection_ptrs.end();) {
            auto& connection = *(*it).get();

            if (connection.descriptor.is_safe_delete()) {
                connection_table.erase(&connection.descriptor);
                it = connection_ptrs.erase(it);
                continue;
            }

            if (connection.descriptor.is_expired())
                connection.descriptor.set_offline();

            ++it;
        }
    }

    void ConnectionEnvironment::activate_pending_connections()
    {
        for (auto connection = pending_connection.pop();
            connection;
            connection = connection->next) {
            auto connection_descriptor = connection->value;
            connection_table.insert(connection_descriptor);

            connection_descriptor->activate_receive_cycle(connection_descriptor->io_recv_event);
        }
    }

    void ConnectionEnvironment::flush_server_message()
    {
        for (const auto& desc : connection_table) {
            for (auto event : desc->io_send_events) {
                if (not event->is_processing)
                    desc->activate_send_cycle(event);
            }
        }
    }

    void ConnectionEnvironment::flush_client_message()
    {
        for (const auto& desc : connection_table) {
            if (desc->is_online() && not desc->io_recv_event->is_processing) {
                desc->io_recv_event->is_processing = true;

                // if there are no unprocessed packets, connection will be close.
                // (because it is unusual situation)
                desc->io_recv_event->invoke_handler(*desc->connection,
                    desc->io_recv_event->data.size() ? io::RETRY_SIGNAL : io::EOF_SIGNAL);
            }
        }
    }
}