#include "pch.h"
#include "connection_environment.h"

#include <algorithm>
#include <cassert>

namespace net
{
    ConnectionEnvironment::ConnectionEnvironment(unsigned max_connections)
        : num_of_max_connections{ max_connections }
        , connection_table{ new ConnectionEntry[max_connections]() }
    {
        
    }

    unsigned ConnectionEnvironment::get_unused_table_index()
    {
        // find first unused slot.
        auto unused_slot = std::find_if(connection_table.get(), connection_table.get() + num_of_max_connections,
            [](const auto& entry) {
                return not entry.used.load(std::memory_order_relaxed);
            }
        );

        assert(unused_slot - connection_table.get() < num_of_max_connections);
        return unsigned(unused_slot - connection_table.get());
    }

    void ConnectionEnvironment::append_connection(win::ObjectPool<net::Connection>::Pointer&& a_connection_ptr)
    {
        ++num_of_connections;

        auto& descriptor = a_connection_ptr.get()->descriptor;

        auto index = get_unused_table_index();
        descriptor.connection_table_index = index;

        // Note: should first activate the event before registering the connection to the table.
        descriptor.emit_receive_event(descriptor.io_recv_event);

        connection_table[index].connection = a_connection_ptr.get();
        connection_table[index].used.store(true, std::memory_order_release);
        connection_table[index].online = true;

        connection_table[index].connection_life = std::move(a_connection_ptr);
    }

    void ConnectionEnvironment::on_connection_delete(unsigned index)
    {
        connection_table[index].connection = nullptr;
        connection_table[index].identity = 0;
        connection_table[index].used.store(false, std::memory_order_release);
    }

    void ConnectionEnvironment::on_connection_offline(unsigned index)
    {
        connection_table[index].online = false;
    }

    void ConnectionEnvironment::cleanup_expired_connection()
    {
        auto deleted_connection_count = std::count_if(connection_table.get(), connection_table.get() + num_of_max_connections,
            [](auto& entry) {
                if (not entry.used) return false;

                auto& desc = entry.connection->descriptor;

                if (desc.is_safe_delete()) {
                    entry.connection_life.reset();
                    return true;
                }

                if (desc.is_expired())
                    desc.set_offline();
                
                return false;
            }
        );

        num_of_connections -= unsigned(deleted_connection_count);
    }

    void ConnectionEnvironment::flush_server_message()
    {
        std::for_each_n(connection_table.get(), num_of_max_connections, 
            [](auto& entry) {
                if (not entry.online) return;

                auto& desc = entry.connection->descriptor;

                for (auto event : desc.io_send_events) {
                    if (not event->is_processing)
                        desc.emit_send_event(event);
                }
            }
        );
    }

    void ConnectionEnvironment::flush_client_message()
    {
        std::for_each_n(connection_table.get(), num_of_max_connections,
            [](auto& entry) {
                if (not entry.online) return;

                auto& desc = entry.connection->descriptor;

                if (not desc.io_recv_event->is_processing) {
                    desc.io_recv_event->is_processing = true;

                    // Note: receive event may stop only for one reason: insuffient buffer space.
                    //       unlike flush_server_message(), it need to invoke the I/O handler to process pending packets.
                    desc.io_recv_event->invoke_handler(*entry.connection,
                    desc.io_recv_event->data.size() ? io::RETRY_SIGNAL : io::EOF_SIGNAL);
                }
            }
        );
    }

    bool ConnectionEnvironment::set_authentication_key(unsigned index, unsigned identity)
    {
        auto is_dulicated_user_not_exist = std::all_of(connection_table.get(), connection_table.get() + num_of_max_connections,
            [identity](const auto& entry) {
                return !entry.identity || entry.identity != identity;
            }
        );

        connection_table[index].identity = identity;

        return is_dulicated_user_not_exist;
    }
}