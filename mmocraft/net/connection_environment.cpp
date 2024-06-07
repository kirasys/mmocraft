#include "pch.h"
#include "connection_environment.h"

namespace net
{
    void ConnectionEnvironment::append_connection(win::ObjectPool<net::Connection>::Pointer&& a_connection_ptr)
    {
        connection_counter.fetch_add(1);
        pending_connections.push(std::move(a_connection_ptr));
    }

    void ConnectionEnvironment::on_connection_delete(Connection* deleted_connection)
    {
        auto& connection_descriptor = deleted_connection->descriptor;
        connection_table.erase(&connection_descriptor);

        // for the thread-safety, append to the lock-free stack first.
        if (connection_descriptor.self_player) {
            delete_pending_players.push(connection_descriptor.self_player->get_identity_number());
        }
    }

    void ConnectionEnvironment::cleanup_expired_connection()
    {
        int deleted_connection_count = 0;

        for (auto it = connection_ptrs.begin(); it != connection_ptrs.end();) {
            auto& connection = *(*it).get();

            if (connection.descriptor.is_safe_delete()) {
                it = connection_ptrs.erase(it); // will invoke on_connecyion_delete()
                ++deleted_connection_count;
                continue;
            }

            if (connection.descriptor.is_expired())
                connection.descriptor.set_offline();

            ++it;
        }

        connection_counter.fetch_sub(deleted_connection_count);
    }

    void ConnectionEnvironment::register_pending_connections()
    {
        for (auto peding_connection_node = pending_connections.pop();
                peding_connection_node;
                peding_connection_node = peding_connection_node->next) {
            auto& connection_ptr = peding_connection_node->value;
            auto& connection_descriptor = connection_ptr.get()->descriptor;

            connection_table.insert(&connection_descriptor);
            connection_descriptor.activate_receive_cycle(connection_descriptor.io_recv_event);

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

    bool ConnectionEnvironment::register_player(game::PlayerID a_player_id)
    {
        if (player_lookup_table.find(a_player_id) != player_lookup_table.end())
            return false; // already associated.

        player_lookup_table.insert(a_player_id);
        return true;
    }

    void ConnectionEnvironment::cleanup_deleted_player()
    {
        for (auto deleted_player_node = delete_pending_players.pop();
                deleted_player_node;
                deleted_player_node = deleted_player_node->next) {
            auto player_id = deleted_player_node->value;
            player_lookup_table.erase(player_id);
        }
    }
}