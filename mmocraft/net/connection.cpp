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
                                io::IoCompletionPort& io_service,
                                io::IoEventPool &io_event_pool)
        : packet_handle_server{ a_packet_handle_server }

        , io_send_event_data{ io_event_pool.new_send_event_data() }
        , io_send_event{ io_event_pool.new_send_event(io_send_event_data.get()) }

        , io_immedidate_send_event_data{ io_event_pool.new_send_event_small_data() }
        , io_immedidate_send_event{ io_event_pool.new_send_event(io_immedidate_send_event_data.get()) }

        , io_recv_event_data{ io_event_pool.new_recv_event_data() }
        , io_recv_event{ io_event_pool.new_recv_event(io_recv_event_data.get()) }

        , descriptor{ 
            this,
            a_connection_key,
            a_connection_env, 
            std::move(sock), 
            io_service,
            io_recv_event.get(), 
            io_immedidate_send_event.get(), 
            io_send_event.get()
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
    
    /// recv event handler

    void Connection::on_complete(io::IoRecvEvent* event)
    {
        if (last_error_code.is_packet_handle_success()) {
            last_error_code.reset();
            descriptor.emit_receive_event(event);
            return;
        }

        descriptor.disconnect_immediate(last_error_code.to_string());
        event->is_processing = false;
    }

    std::size_t Connection::handle_io_event(io::IoRecvEvent* event)
    {
        if (not descriptor.try_interact_with_client()) // timeout case: connection will be deleted soon.
            return 0; 

        return process_packets(event->data.begin(), event->data.end());
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
        io::IoSendEvent* a_io_immediate_send_event,
        io::IoSendEvent* a_io_deferred_send_event)
        : _connection_key{ a_connection_key }
        , connection_env{ a_connection_env }
        , client_socket{ std::move(a_sock) }
        , io_recv_event{ a_io_recv_event }
        , io_send_events{ a_io_immediate_send_event, a_io_deferred_send_event }
        , online{ true }
    {
        update_last_interaction_time();

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
        if (not online || event->data.unused_size() < PacketStructure::max_size_of_packet_struct()) {
            event->is_processing = false;
            return;
        }

        WSABUF wbuf[1] = {};
        wbuf[0].buf = reinterpret_cast<char*>(event->data.begin_unused());
        wbuf[0].len = ULONG(event->data.unused_size());

        // should assign flag first to avoid data race.
        event->is_processing = true;
        if (not client_socket.recv(&event->overlapped, wbuf))
            event->is_processing = false;
    }

    void Connection::Descriptor::emit_send_event(io::IoSendEvent* event)
    {
        if (event->data.size() == 0)
            return;

        WSABUF wbuf[1] = {};
        wbuf[0].buf = reinterpret_cast<char*>(event->data.begin());
        wbuf[0].len = ULONG(event->data.size());

        // should assign flag first to avoid data race.
        event->is_processing = true; 
        if (not client_socket.send(&event->overlapped, wbuf))
            event->is_processing = false;
    }

    bool Connection::Descriptor::disconnect_immediate(std::string_view reason)
    {
        if (not online)
            return false;

        net::PacketDisconnectPlayer disconnect_packet{ reason };
        auto result = disconnect_packet.serialize(io_send_events[SendType::IMMEDIATE]->data);

        set_offline();
        emit_send_event(io_send_events[SendType::IMMEDIATE]); // TODO: resolve interleaving problem.

        return result;
    }

    bool Connection::Descriptor::disconnect_deferred(std::string_view reason)
    {
        net::PacketDisconnectPlayer disconnect_packet{ reason };
        auto result = disconnect_packet.serialize(io_send_events[SendType::DEFERRED]->data);
    
        set_offline();
        emit_send_event(io_send_events[SendType::DEFERRED]); // TODO: resolve interleaving problem.

        return result;
    }

    bool Connection::Descriptor::send_handshake_packet(const net::PacketHandshake& packet, SendType send_type) const
    {
        return packet.serialize(io_send_events[send_type]->data);
    }

    void Connection::Descriptor::flush_send(net::ConnectionEnvironment& connection_env)
    {
        auto flush_message = [](Connection::Descriptor& desc) {
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
                        conn.descriptor.io_recv_event->data.size() ? io::RETRY_SIGNAL : io::EOF_SIGNAL);
                }
            }
        };

        connection_env.for_each_connection(flush_message);
    }
}