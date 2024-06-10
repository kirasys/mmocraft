#include "pch.h"
#include "connection_environment.h"

#include <cassert>

namespace net
{
    ConnectionEnvironment::ConnectionEnvironment()
    {
        const auto& conf = config::get_config();

        num_of_max_connections = conf.server.max_player;
        connection_table.reserve(num_of_max_connections);
        player_lookup_table.reset(new unsigned[num_of_max_connections]);
    }

    void ConnectionEnvironment::append_connection(win::ObjectPool<net::Connection>::Pointer&& a_connection_ptr)
    {
        ++num_of_connections;
        pending_connections.push(std::move(a_connection_ptr));
    }

    void ConnectionEnvironment::on_connection_delete(Connection& deleted_connection)
    {
        auto& connection_descriptor = deleted_connection.descriptor;
        connection_table.erase(&connection_descriptor);

        if (connection_descriptor.self_player)
            player_lookup_table[connection_descriptor.self_player->get_id()] = 0;
    }

    void ConnectionEnvironment::cleanup_expired_connection()
    {
        int deleted_connection_count = 0;

        for (auto it = connection_ptrs.begin(); it != connection_ptrs.end();) {
            auto& connection = *(*it).get();

            if (connection.descriptor.is_safe_delete()) {
                on_connection_delete(connection);
                it = connection_ptrs.erase(it);
                ++deleted_connection_count;
                continue;
            }

            if (connection.descriptor.is_expired())
                connection.descriptor.set_offline();

            ++it;
        }

        num_of_connections -= deleted_connection_count;
    }

    void ConnectionEnvironment::register_pending_connections()
    {
        auto pending_connection_head = pending_connections.pop();

        for (auto connection_node = pending_connection_head.get(); connection_node; connection_node = connection_node->next) {
            auto& connection_ptr = connection_node->value;
            auto& connection_descriptor = connection_ptr.get()->descriptor;

            // should first activate the event before inserting the connection to the table.
            connection_descriptor.activate_receive_cycle(connection_descriptor.io_recv_event);

            connection_table.insert(&connection_descriptor);
            connection_ptrs.emplace_back(std::move(connection_ptr));
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

                // Note: receive event may stop only for one reason: insuffient buffer space.
                //       unlike flush_server_message(), it need to invoke the I/O handler to process pending packets.
                desc->io_recv_event->invoke_handler(*desc->connection,
                    desc->io_recv_event->data.size() ? io::RETRY_SIGNAL : io::EOF_SIGNAL);
            }
        }
    }

    std::pair<game::PlayerID, bool> ConnectionEnvironment::register_player(unsigned player_identity)
    {
        auto empty_slot = num_of_max_connections;

        for (unsigned i = 0; i < num_of_max_connections; i++) {
            // check if the user is already logged in.
            if (player_lookup_table[i] == player_identity)
                return { 0, false };
            // find first unused slot.
            if (player_lookup_table[i] == 0 && empty_slot == num_of_max_connections)
                empty_slot = i;
        }

        assert(empty_slot < num_of_max_connections);
        player_lookup_table[empty_slot] = player_identity;

        return { empty_slot, true };
    }
}