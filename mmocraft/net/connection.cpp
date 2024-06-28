#include "pch.h"
#include "connection.h"

#include <vector>

#include "config/config.h"
#include "proto/config.pb.h"
#include "logging/error.h"
#include "net/packet.h"
#include "net/server_core.h"
#include "net/connection_environment.h"

namespace net
{
    Connection::Connection(net::PacketHandleServer& a_packet_handle_server,
                                net::ConnectionKey a_connection_key,
                                net::ConnectionEnvironment& a_connection_env,
                                win::UniqueSocket&& sock,
                                io::IoService& io_service,
                                io::IoEventPool &io_event_pool)
        : packet_handle_server{ a_packet_handle_server }

        , send_event_data{ io_event_pool.new_send_event_data() }
        , send_event_lockfree_data{ io_event_pool.new_send_event_lockfree_data() }

        , send_events{
            io_event_pool.new_send_event(send_event_data.get()),
            io_event_pool.new_send_event(send_event_lockfree_data.get()),
        }

        , io_recv_event_data{ io_event_pool.new_recv_event_data() }
        , io_recv_event{ io_event_pool.new_recv_event(io_recv_event_data.get()) }

        , descriptor{ 
            this,
            a_connection_key,
            a_connection_env, 
            std::move(sock), 
            io_service,
            io_recv_event.get(), 
            send_events
        }
    {
        if (not is_valid())
            throw error::ErrorCode::CLIENT_CONNECTION_CREATE;
    }

    std::size_t Connection::process_packets(std::byte* data_begin, std::byte* data_end)
    {
        auto data_cur = data_begin;
        auto packet_ptr = static_cast<Packet*>(_alloca(PacketStructure::max_size_of_packet_struct()));

        while (data_cur < data_end) {
            auto [parsed_bytes, parsing_result] = PacketStructure::parse_packet(data_cur, data_end, packet_ptr);
            if (not parsing_result.is_success()) {
                last_error_code = parsing_result;
                break;
            }

            auto handle_result = packet_handle_server.handle_packet(descriptor, packet_ptr);
            if (not handle_result.is_packet_handle_success()) {
                last_error_code = handle_result;
                break;
            }

            data_cur += parsed_bytes;
        }

        assert(data_cur <= data_end && "Parsing error");
        return data_cur - data_begin; // num of total parsed bytes.
    }

    /**
     *  Event Handler Interface
     */
    
    void Connection::on_error()
    {
        descriptor.disconnect();
    }

    /// recv event handler

    void Connection::on_complete(io::IoRecvEvent* event)
    {
        if (last_error_code.is_packet_handle_success()) {
            last_error_code.reset();
            descriptor.emit_receive_event(event);
            return;
        }

        descriptor.send_disconnect_message(ThreadType::Any_Thread, last_error_code.to_string());
        event->is_processing = false;
    }

    std::size_t Connection::handle_io_event(io::IoRecvEvent* event)
    {
        if (not descriptor.try_interact_with_client()) // timeout case: connection will be deleted soon.
            return 0; 

        return process_packets(event->data->begin(), event->data->end());
    }

    /// send event handler

    void Connection::on_complete(io::IoSendEvent* event)
    {
        // Note: Can't start I/O event again due to the interleaving problem.
        event->is_processing = false;
    }

    /**
     *  Connection descriptor interface
     */

    Connection::Descriptor::Descriptor(net::Connection* a_connection,
        net::ConnectionKey a_connection_key,
        net::ConnectionEnvironment& a_connection_env,
        win::UniqueSocket&& a_sock,
        io::IoService& a_io_service,
        io::IoRecvEvent* a_io_recv_event,
        win::ObjectPool<io::IoSendEvent>::Pointer send_events[])
        : _connection_key{ a_connection_key }
        , connection_env{ a_connection_env }
        , client_socket{ std::move(a_sock) }
        , io_recv_event{ a_io_recv_event }
        , io_send_events{ 
            send_events[ThreadType::Tick_Thread].get(),
            send_events[ThreadType::Any_Thread].get(),
        }
        , online{ true }
    {
        update_last_interaction_time();

        // pre-assign fixed length multicast events.
        for (unsigned i = 0; i < num_of_multicast_event; i++)
            io_multicast_send_events.emplace_back(nullptr);

        // start to service client socket events.
        a_io_service.register_event_source(client_socket.get_handle(), a_connection);
        emit_receive_event(io_recv_event);
    }

    Connection::Descriptor::~Descriptor()
    {
        connection_env.on_connection_delete(_connection_key);
    }

    void Connection::Descriptor::set_offline(std::size_t current_tick)
    {
        online = false;
        last_offline_tick = current_tick;
        connection_env.on_connection_offline(_connection_key);
    }

    void Connection::Descriptor::disconnect()
    {
        // player manager(aka. world) has some extra works for disconnecting players.
        // in this case, return without setting offline the connection.
        if (_player && _player->state() >= game::PlayerState::Spawned) {
            _player->set_state(game::PlayerState::Disconnected);
            return;
        }

        set_offline();
    }

    bool Connection::Descriptor::is_expired(std::size_t current_tick) const
    {
        return current_tick >= last_interaction_tick + REQUIRED_MILLISECONDS_FOR_EXPIRE;
    }

    bool Connection::Descriptor::is_safe_delete(std::size_t current_tick) const
    {
        return not online
            && current_tick >= last_offline_tick + REQUIRED_MILLISECONDS_FOR_SECURE_DELETION;
    }

    bool Connection::Descriptor::try_interact_with_client()
    {
        if (online) {
            update_last_interaction_time();
            return true;
        }
        return false;
    }

    void Connection::Descriptor::emit_receive_event(io::IoRecvEvent* event)
    {
        if (not online || event->data->unused_size() < PacketStructure::max_size_of_packet_struct()) {
            event->is_processing = false;
            return;
        }

        WSABUF wbuf[1] = {};
        wbuf[0].buf = reinterpret_cast<char*>(event->data->begin_unused());
        wbuf[0].len = ULONG(event->data->unused_size());

        // should assign flag first to avoid data race.
        event->is_processing = true;
        if (not client_socket.recv(&event->overlapped, wbuf))
            event->is_processing = false;
    }

    void Connection::Descriptor::emit_send_event(io::IoSendEvent* event)
    {
        WSABUF wbuf[1] = {};
        wbuf[0].buf = reinterpret_cast<char*>(event->data->begin());
        wbuf[0].len = ULONG(event->data->size());

        if (wbuf[0].len) {
            std::lock_guard<std::mutex> lock(send_event_lock);

            // should assign flag first to avoid data race.
            event->is_processing = true;
            if (not client_socket.send(&event->overlapped, wbuf))
                event->is_processing = false;
        }
    }

    bool Connection::Descriptor::emit_multicast_send_event(io::IoSendEventSharedData* event_data)
    {
        WSABUF wbuf[1] = {};
        wbuf[0].buf = reinterpret_cast<char*>(event_data->begin());
        wbuf[0].len = ULONG(event_data->size());

        if (wbuf[0].len) {
            std::lock_guard<std::mutex> lock(send_event_lock);

            auto unused_event = std::find_if(io_multicast_send_events.begin(), io_multicast_send_events.end(),
                [](auto& event) {
                    return not event.is_processing;
                }
            );

            if (unused_event == io_multicast_send_events.end()) {
                LOG(error) << "Insufficient multicast events.";
                return false;
            }

            unused_event->set_data(event_data);

            // should assign flag first to avoid data race.
            unused_event->is_processing = true;
            if (not client_socket.send(&unused_event->overlapped, wbuf))
                return unused_event->is_processing = false;
        }
        return true;
    }

    void Connection::Descriptor::on_handshake_success(game::Player* player)
    {
        const auto& server_conf = config::get_server_config();

        net::PacketHandshake handshake_packet{
            server_conf.server_name(), server_conf.motd(),
            player->player_type() == game::PlayerType::ADMIN ? net::UserType::OP : net::UserType::NORMAL
        };
        net::PacketLevelInit level_init_packet;

        send_packet(ThreadType::Any_Thread, handshake_packet);
        send_packet(ThreadType::Any_Thread, level_init_packet);

        _player = player;
        _player->set_state(game::PlayerState::Handshake_Completed);
    }

    bool Connection::Descriptor::send_raw_data(ThreadType sender_type, const std::byte* data, std::size_t data_size) const
    {
        return io_send_events[sender_type]->data->push(data, data_size);
    }

    bool Connection::Descriptor::send_disconnect_message(ThreadType sender_type, std::string_view reason)
    {
        net::PacketDisconnectPlayer disconnect_packet{ reason };
        bool result = disconnect_packet.serialize(*io_send_events[sender_type]->data);

        disconnect();
        emit_send_event(io_send_events[sender_type]); // TODO: resolve interleaving problem.

        return result;
    }

    bool Connection::Descriptor::send_disconnect_message(ThreadType sender_type, error::ResultCode result)
    {
        return send_disconnect_message(sender_type, result.to_string());
    }

    bool Connection::Descriptor::send_ping(ThreadType sender_type) const
    {
        auto packet = std::byte(net::PacketID::Ping);
        return io_send_events[sender_type]->data->push(&packet, 1);
    }

    bool Connection::Descriptor::send_packet(ThreadType sender_type, const net::PacketHandshake& packet) const
    {
        return packet.serialize(*io_send_events[sender_type]->data);
    }

    bool Connection::Descriptor::send_packet(ThreadType sender_type, const net::PacketLevelInit& packet) const
    {
        return packet.serialize(*io_send_events[sender_type]->data);
    }

    bool Connection::Descriptor::send_packet(ThreadType sender_type, const net::PacketSetPlayerID& packet) const
    {
        return packet.serialize(*io_send_events[sender_type]->data);
    }

    bool Connection::Descriptor::send_packet(ThreadType sender_type, const net::PacketSetBlockServer& packet) const
    {
        return packet.serialize(*io_send_events[sender_type]->data);
    }

    void Connection::Descriptor::flush_send(net::ConnectionEnvironment& connection_env)
    {
        auto flush_message = [](Connection::Descriptor& desc) {
            // flush immedidate and deferred messages.
            for (auto event : desc.io_send_events) {
                if (not event->is_processing)
                    desc.emit_send_event(event);
            }
        };

        connection_env.for_each_descriptor(flush_message);
    }

    void Connection::Descriptor::flush_receive(net::ConnectionEnvironment& connection_env)
    {
        auto flush_message = [](Connection& conn) {
            for (auto event : conn.descriptor.io_send_events) {
                if (not conn.descriptor.io_recv_event->is_processing) {
                    conn.descriptor.io_recv_event->is_processing = true;

                    // Note: receive event may stop only for one reason: insuffient buffer space.
                    //       unlike flush_server_message(), it need to invoke the I/O handler to process pending packets.
                    conn.descriptor.io_recv_event->invoke_handler(conn,
                        conn.descriptor.io_recv_event->data->size() ? io::RETRY_SIGNAL : io::EOF_SIGNAL,
                        ERROR_SUCCESS);
                }
            }
        };

        connection_env.for_each_connection(flush_message);
    }
}