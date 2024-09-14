#include "pch.h"
#include "connection.h"

#include <vector>

#include "config/config.h"
#include "logging/error.h"
#include "net/tcp_server.h"
#include "net/connection_environment.h"

namespace net
{
    Connection::Connection(net::PacketHandleServer& a_packet_handle_server,
                                net::ConnectionKey a_connection_key,
                                net::ConnectionEnvironment& a_connection_env,
                                win::UniqueSocket&& sock,
                                io::RegisteredIO& io_service)
        : packet_handle_server{ a_packet_handle_server }
        , _connection_key{ a_connection_key }
        , connection_env{ a_connection_env }
        , _is_online{ true }
        , connection_io { new ConnectionIO(this, io_service, std::move(sock)) }
    {
        if (not is_valid())
            throw error::ErrorCode::CLIENT_CONNECTION_CREATE;

        update_last_interaction_time();
    }

    Connection::~Connection()
    {
        connection_env.on_connection_delete(_connection_key);
    }

    void Connection::set_offline(std::size_t current_tick)
    {
        _is_online = false;
        last_offline_tick = current_tick;
        connection_env.on_connection_offline(_connection_key);
    }

    bool Connection::is_expired(std::size_t current_tick) const
    {
        return current_tick >= last_interaction_tick + REQUIRED_MILLISECONDS_FOR_EXPIRE;
    }

    bool Connection::is_safe_delete(std::size_t current_tick) const
    {
        return not _is_online
            && current_tick >= last_offline_tick + REQUIRED_MILLISECONDS_FOR_SECURE_DELETION;
    }

    bool Connection::try_interact_with_client()
    {
        if (_is_online) {
            update_last_interaction_time();
            return true;
        }
        return false;
    }

    void Connection::disconnect()
    {
        // already disconencted.
        if (not is_online() || (_player && _player->state() == game::PlayerState::Disconnect_Wait))
           return;

        // player manager(aka. world) has some extra works for disconnecting players.
        // in this case, return without setting offline the connection.
        if (_player && _player->state() >= game::PlayerState::Spawned) {
            _player->set_state(game::PlayerState::Disconnect_Wait);
            return;
        }
        
        packet_handle_server.on_disconnect(*this);
        set_offline();
    }

    void Connection::kick(std::string_view message)
    {
        net::PacketDisconnectPlayer disconnect_packet{ message };
        connection_io->send_packet(disconnect_packet);

        _is_kicked = true;
    }

    void Connection::kick(error::ResultCode result)
    {
        kick(result.to_string());
    }

    std::size_t Connection::process_packets(std::byte* data_begin, std::byte* data_end)
    {
        auto data_cur = data_begin;

        while (data_cur < data_end) {
            auto [packet_id, packet_size] = PacketStructure::parse_packet(data_cur);

            if (packet_id == net::PacketID::INVALID) {
                last_error_code = error::PACKET_INVALID_ID;
                break;
            }

            if (packet_size > data_end - data_cur) {
                last_error_code = error::PACKET_INSUFFIENT_DATA;
                break;
            }

            auto handle_result = packet_handle_server.handle_packet(*this, data_cur);
            if (not handle_result.is_packet_handle_success()) {
                last_error_code = handle_result;
                break;
            }

            data_cur += packet_size;
        }

        assert(data_cur <= data_end && "Parsing error");
        return data_cur - data_begin; // num of total parsed bytes.
    }

    void Connection::on_handshake_success()
    {
        assert(_player != nullptr);
        const auto& conf = config::get_config();

        net::PacketHandshake handshake_packet{
            conf.tcp_server().server_name(), conf.tcp_server().motd(),
            _player->player_type() == game::PlayerType::ADMIN ? net::UserType::OP : net::UserType::NORMAL
        };
        net::PacketLevelInit level_init_packet;

        connection_io->send_packet(handshake_packet);
        connection_io->send_packet(level_init_packet);

        _player->set_state(game::PlayerState::Handshake_Completed);
    }


    void Connection::send_supported_cpe_list()
    {
        net::PacketExtInfo ext_info_packet;
        connection_io->send_packet(ext_info_packet);

        net::PacketExtEntry ext_entry_packet;
        connection_io->send_packet(ext_entry_packet);
    }


    /**
     *  Event Handler Interface
     */
    
    void Connection::on_error()
    {
        disconnect();
    }

    /// recv event handler

    void Connection::on_complete(io::IoRecvEvent* event)
    {
        event->is_processing = false;

        if (not last_error_code.is_packet_handle_success()) {
            kick(last_error_code.to_string());
            return;
        }

        last_error_code.reset();
    }

    std::size_t Connection::handle_io_event(io::IoRecvEvent* event)
    {
        if (not try_interact_with_client()) // timeout case: connection will be deleted soon.
            return 0; 

        return process_packets(event->event_data()->begin(), event->event_data()->end());
    }

    /// send event handler

    void Connection::on_complete(io::IoSendEvent* event, std::size_t transferred_bytes)
    {
        // Note: Can't start I/O event again due to the interleaving problem.
        event->is_processing = false;
    }

    void Connection::on_complete(io::IoMulticastSendEvent* event, std::size_t transferred_bytes)
    {
        event->is_processing = false;
        event->reset_multicast_data();

        // connection_io manages all multicast events.
        connection_io->on_complete_event(event);
    }

    /**
     *  Connection descriptor interface
     */

    ConnectionIO::ConnectionIO(net::Connection* conn, io::RegisteredIO& a_io_service, win::UniqueSocket&& a_sock)
        : io_service{ a_io_service }
        , connection_id{ conn->connection_id() }
        , client_socket{ std::move(a_sock) }
        , io_recv_event{ io_service.create_recv_io_event(connection_id) }
        , io_send_event{ io_service.create_send_io_event(connection_id) }
    {
        // start to service client socket events.
        io_service.register_event_source(connection_id, client_socket.get_handle(), conn);
    }

    ConnectionIO::~ConnectionIO()
    {

    }

    void ConnectionIO::close()
    {
        client_socket.close();
    }

    bool ConnectionIO::post_recv_event()
    {
        return io_recv_event->post_rio_event(io_service, connection_id);
    }

    bool ConnectionIO::post_send_event()
    {
        return io_send_event->post_rio_event(io_service, connection_id);
    }

    bool ConnectionIO::send_raw_data(const std::byte* data, std::size_t data_size) const
    {
        return io_send_event->event_data()->push(data, data_size);
    }

    void ConnectionIO::send_multicast_data(io::MulticastDataEntry& entry)
    {
        // Use send buffer if connection has suffient space.
        if (entry.data_size() < io_send_event->event_data()->unused_size() / 2 &&
            send_raw_data(entry.data(), entry.data_size())) {
            return;
        }

        io::IoMulticastSendEvent* io_multicast_event = nullptr;

        if (free_multicast_events.empty()) {
            io_multicast_event = new io::IoMulticastSendEvent();
        }

        {
            std::lock_guard<std::mutex> lock(multicast_event_lock);
            if (io_multicast_event == nullptr) {
                io_multicast_event = free_multicast_events.back();
                free_multicast_events.pop_back();
            }

            io_multicast_event->set_multicast_data(&entry);
            ready_multicast_events.push_back(io_multicast_event);
        }
    }

    bool ConnectionIO::send_ping() const
    {
        auto packet = std::byte(net::PacketID::Ping);
        return io_send_event->event_data()->push(&packet, net::PacketPing::packet_size);
    }

    void ConnectionIO::flush_send(net::ConnectionEnvironment& connection_env)
    {
        auto flush_message = [](Connection& conn) {
            auto connection_io = conn.io();

            if (not connection_io->is_send_io_busy())
                connection_io->post_send_event();

            connection_io->flush_multicast_send();

            // Disconnect if a kick message has been sent.
            if (conn.is_kicked())
                conn.disconnect();
        };

        connection_env.for_each_connection(flush_message);
    }

    void ConnectionIO::flush_multicast_send()
    {
        std::vector<io::IoMulticastSendEvent*> multicast_events;
        multicast_events.reserve(max_multicast_event_count);

        {
            std::lock_guard<std::mutex> lock(multicast_event_lock);
            multicast_events.swap(ready_multicast_events);
        }

        for (auto event : multicast_events) {
            if (not event->post_rio_event(io_service, connection_id))
                delete event;
        }
    }

    void ConnectionIO::flush_receive(net::ConnectionEnvironment& connection_env)
    {
        auto flush_message = [](Connection& conn) {
            auto connection_io = conn.io();

            if (not connection_io->is_receive_io_busy())
                connection_io->post_recv_event();
        };

        connection_env.for_each_connection(flush_message);
    }



    void ConnectionIO::on_complete_event(io::IoMulticastSendEvent* event)
    {
        if (free_multicast_events.size() >= max_multicast_event_count)
            delete event;
        else {
            std::lock_guard<std::mutex> lock(multicast_event_lock);
            free_multicast_events.push_back(event);
        }
    }
}