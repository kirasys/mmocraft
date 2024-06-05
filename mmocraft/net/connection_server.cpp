#include "pch.h"
#include "connection_server.h"

#include <vector>

#include "logging/error.h"
#include "net/packet.h"
#include "net/server_core.h"

namespace net
{
    util::IntervalTaskScheduler<void> ConnectionServer::interval_task_scheduler;
    util::LockfreeStack<ConnectionServer::Descriptor*> ConnectionServer::accepted_connections;
    std::unordered_map<ConnectionServer::Descriptor*, bool> ConnectionServer::online_connection_table;
    std::unordered_map<game::PlayerID, game::Player*> ConnectionServer::player_lookup_table;

    ConnectionServer::ConnectionServer(net::PacketHandleServer& a_packet_handle_server,
                                win::UniqueSocket&& sock,
                                io::IoCompletionPort& io_service,
                                io::IoEventPool &io_event_pool)
        : packet_handle_server{ a_packet_handle_server }
        , _client_socket{ std::move(sock) }

        , io_send_event_data{ io_event_pool.new_send_event_data() }
        , io_send_event{ io_event_pool.new_send_event(io_send_event_data.get()) }

        , io_immedidate_send_event_data{ io_event_pool.new_send_event_small_data() }
        , io_immedidate_send_event{ io_event_pool.new_send_event(io_immedidate_send_event_data.get()) }

        , io_recv_event_data{ io_event_pool.new_recv_event_data() }
        , io_recv_event{ io_event_pool.new_recv_event(io_recv_event_data.get()) }
    {
        if (not is_valid())
            throw error::ErrorCode::CLIENT_CONNECTION_CREATE;

        // allow to service client socket events.
        io_service.register_event_source(_client_socket.get_handle(), this);

        // register the descriptor.
        register_descriptor();
    }

    void ConnectionServer::register_descriptor()
    {
        connection_descriptor.connection = this;
        connection_descriptor.raw_socket = _client_socket.get_handle();
        connection_descriptor.io_recv_event = io_recv_event.get();
        connection_descriptor.io_send_events[SendType::IMMEDIATE] = io_immedidate_send_event.get();
        connection_descriptor.io_send_events[SendType::DEFERRED] = io_send_event.get();

        connection_descriptor.is_online = true;
        connection_descriptor.update_last_interaction_time();

        accepted_connections.push(&this->connection_descriptor);
    }

    ConnectionServer::~ConnectionServer()
    {
        online_connection_table.erase(&connection_descriptor);
        // For thread-safety, erase the entry at the tick routine.
        player_lookup_table[connection_descriptor.self_player->get_identity_number()] = nullptr;
    }

    void ConnectionServer::initialize_system()
    {
        const auto& conf = config::get_config();

        online_connection_table.reserve(conf.server.max_player);
        player_lookup_table.reserve(conf.server.max_player);

        /// Schedule interval tasks.

        interval_task_scheduler.schedule(
            util::TaskTag::CLEAN_CONNECTION,
            &ServerCore::cleanup_expired_connection,
            util::MilliSecond(10000));

        interval_task_scheduler.schedule(
            util::TaskTag::CLEAN_PLAYER,
            &ConnectionServer::cleanup_deleted_player,
            util::MilliSecond(10000));
    }

    bool ConnectionServer::is_expired(std::size_t current_tick) const
    {
        return current_tick >= connection_descriptor.last_interaction_tick + REQUIRED_MILLISECONDS_FOR_EXPIRE;
    }

    bool ConnectionServer::is_safe_delete(std::size_t current_tick) const
    {
        return not connection_descriptor.is_online
            && current_tick >= connection_descriptor.last_offline_tick + REQUIRED_MILLISECONDS_FOR_SECURE_DELETION;
    }

    void ConnectionServer::set_offline()
    {
        connection_descriptor.set_offline();
    }

    std::size_t ConnectionServer::process_packets(std::byte* data_begin, std::byte* data_end)
    {
        auto data_cur = data_begin;
        auto packet_ptr = static_cast<Packet*>(_alloca(PacketStructure::max_size_of_packet_struct()));

        while (data_cur < data_end) {
            auto [parsed_bytes, parsing_result] = PacketStructure::parse_packet(data_cur, data_end, packet_ptr);
            if (not parsing_result.is_success()) {
                last_error_code = parsing_result;
                break;
            }

            auto handle_result = packet_handle_server.handle_packet(connection_descriptor, packet_ptr);
            if (not handle_result.is_success()) {
                last_error_code = handle_result;
                break;
            }

            data_cur += parsed_bytes;
        }

        assert(data_cur <= data_end && "Parsing error");
        return data_cur - data_begin; // num of total parsed bytes.
    }

    void ConnectionServer::tick()
    {
        // clean-up expired connections.
        interval_task_scheduler.process_task(util::TaskTag::CLEAN_CONNECTION);
        activate_pending_connections();

        flush_server_message();
        flush_client_message();
    }

    void ConnectionServer::activate_pending_connections()
    {
        for (auto connection = accepted_connections.pop();
                connection;
                connection = connection->next) {
            auto connection_descriptor = connection->value;
            online_connection_table[connection_descriptor] = true;
            connection_descriptor->activate_receive_cycle(connection_descriptor->io_recv_event);
        }
    }

    void ConnectionServer::cleanup_deleted_player()
    {
        std::vector<game::PlayerID> deleted_player;

        for (const auto& [player_id, player] : player_lookup_table)
        {
            if (player == nullptr)
                deleted_player.push_back(player_id);
        }

        for (auto player_id : deleted_player) {
            player_lookup_table.erase(player_id);
        }
    }

    void ConnectionServer::flush_server_message()
    {
        for (const auto& [desc, online] : online_connection_table) {
            for (auto event : desc->io_send_events) {
                if (not event->is_processing)
                    desc->activate_send_cycle(event);
            }
        }
    }

    void ConnectionServer::flush_client_message()
    {
        for (const auto& [desc, online] : online_connection_table) {
            if (online && not desc->io_recv_event->is_processing) {
                desc->io_recv_event->is_processing = true;

                // if there are no unprocessed packets, connection will be close.
                // (because it is unusual situation)
                desc->io_recv_event->invoke_handler(*desc->connection,
                    desc->io_recv_event->data.size() ? io::RETRY_SIGNAL : io::EOF_SIGNAL);
            }
        }
    }

    /**
     *  Event Handler Interface
     */
    
    /// recv event handler

    void ConnectionServer::on_complete(io::IoRecvEvent* event)
    {
        if (last_error_code.is_strong_success()) {
            connection_descriptor.activate_receive_cycle(event);
            return;
        }

        connection_descriptor.disconnect_immediate(last_error_code.to_string());
        event->is_processing = false;
    }

    std::size_t ConnectionServer::handle_io_event(io::IoRecvEvent* event)
    {
        if (not connection_descriptor.try_interact_with_client()) // timeout case: connection will be deleted soon.
            return 0; 

        return process_packets(event->data.begin(), event->data.end());
    }

    /// send event handler

    void ConnectionServer::on_complete(io::IoSendEvent* event)
    {
        // Note: Can't start I/O event again due to the interleaving problem.
        event->is_processing = false;
    }

    /**
     *  Connection descriptor interface
     */

    bool ConnectionServer::Descriptor::associate_game_player
        (game::PlayerID player_id, game::PlayerType player_type, const char* username, const char* password)
    {
        // clean up deleted players.
        interval_task_scheduler.process_task(util::TaskTag::CLEAN_PLAYER);

        if (not is_online || player_lookup_table.find(player_id) != player_lookup_table.end())
            return false; // already associated.

        auto player_ptr = std::make_unique<game::Player>(
            player_id,
            player_type,
            username,
            password
        );

        player_lookup_table[player_id] = player_ptr.get();
        self_player = std::move(player_ptr);

        return true;
    }

    void ConnectionServer::Descriptor::set_offline()
    {
        online_connection_table[this] = false;
        last_offline_tick = util::current_monotonic_tick();
    }

    bool ConnectionServer::Descriptor::try_interact_with_client()
    {
        if (is_online) {
            update_last_interaction_time();
            return true;
        }
        return false;
    }

    void ConnectionServer::Descriptor::activate_receive_cycle(io::IoRecvEvent* event)
    {
        if (not is_online || io_recv_event->data.unused_size() < PacketStructure::max_size_of_packet_struct()) {
            event->is_processing = false;
            return;
        }

        WSABUF wbuf[1] = {};
        wbuf[0].buf = reinterpret_cast<char*>(io_recv_event->data.begin_unused());
        wbuf[0].len = ULONG(io_recv_event->data.unused_size());

        // should assign flag first to avoid data race.
        event->is_processing = true;
        if (not Socket::recv(raw_socket, &io_recv_event->overlapped, wbuf, 1))
            event->is_processing = false;
    }

    void ConnectionServer::Descriptor::activate_send_cycle(io::IoSendEvent* event)
    {
        if (event->data.size() == 0)
            return;

        WSABUF wbuf[1] = {};
        wbuf[0].buf = reinterpret_cast<char*>(event->data.begin());
        wbuf[0].len = ULONG(event->data.size());

        // should assign flag first to avoid data race.
        event->is_processing = true; 
        if (not Socket::send(raw_socket, &event->overlapped, wbuf, 1))
            event->is_processing = false;
    }

    bool ConnectionServer::Descriptor::disconnect_immediate(std::string_view reason)
    {
        if (not is_online)
            return false;

        net::PacketDisconnectPlayer disconnect_packet{ reason };
        auto result = disconnect_packet.serialize(io_send_events[SendType::IMMEDIATE]->data);

        set_offline();
        return result;
    }

    bool ConnectionServer::Descriptor::disconnect_deferred(std::string_view reason)
    {
        if (not is_online)
            return false;

        net::PacketDisconnectPlayer disconnect_packet{ reason };
        auto result = disconnect_packet.serialize(io_send_events[SendType::DEFERRED]->data);
    
        set_offline();
        return result;
    }

    bool ConnectionServer::Descriptor::finalize_handshake() const
    {
        if (not is_online)
            return false;

        const auto& conf = config::get_config();

        PacketHandshake handshake_packet{
            conf.server.server_name, conf.server.motd, 
            self_player->get_player_type() == game::PlayerType::ADMIN ? net::UserType::OP : net::UserType::NORMAL
        };

        return handshake_packet.serialize(io_send_events[SendType::DEFERRED]->data);
    }
}