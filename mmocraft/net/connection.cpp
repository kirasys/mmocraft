#include "pch.h"
#include "connection.h"

#include <vector>

#include "logging/error.h"
#include "net/packet.h"
#include "net/server_core.h"
#include "net/connection_environment.h"

namespace net
{
    template <class T>
    TConnection<T>::TConnection(net::TPacketHandleServer<T>& a_packet_handle_server,
                                net::ConnectionEnvironment& a_connection_env,
                                win::UniqueSocket&& sock,
                                io::IoCompletionPort& io_service,
                                io::IoEventPool &io_event_pool)
        : descriptor{ std::move(sock) }
        , connection_env{ a_connection_env }
        , packet_handle_server{ a_packet_handle_server }

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
        io_service.register_event_source(descriptor.client_socket.get_handle(), this);

        // register the descriptor.
        register_descriptor();
    }

    template <class T>
    void TConnection<T>::register_descriptor()
    {
        descriptor.connection = this;
        descriptor.io_recv_event = io_recv_event.get();
        descriptor.io_send_events[SendType::IMMEDIATE] = io_immedidate_send_event.get();
        descriptor.io_send_events[SendType::DEFERRED] = io_send_event.get();

        descriptor.online = true;
        descriptor.update_last_interaction_time();
    }

    template <class T>
    TConnection<T>::~TConnection()
    {
        
    }

    template <class T>
    std::size_t TConnection<T>::process_packets(std::byte* data_begin, std::byte* data_end)
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
            if (not handle_result.is_success()) {
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

    template <class T>
    void TConnection<T>::on_complete(io::IoRecvEvent* event)
    {
        if (last_error_code.is_strong_success()) {
            descriptor.activate_receive_cycle(event);
            return;
        }

        descriptor.disconnect_immediate(last_error_code.to_string());
        event->is_processing = false;
    }

    template <class T>
    std::size_t TConnection<T>::handle_io_event(io::IoRecvEvent* event)
    {
        if (not descriptor.try_interact_with_client()) // timeout case: connection will be deleted soon.
            return 0; 

        return process_packets(event->data.begin(), event->data.end());
    }

    /// send event handler

    template <class T>
    void TConnection<T>::on_complete(io::IoSendEvent* event)
    {
        // Note: Can't start I/O event again due to the interleaving problem.
        event->is_processing = false;
    }

    /**
     *  Connection descriptor interface
     */

    template <class T>
    bool TConnection<T>::Descriptor::is_expired(std::size_t current_tick) const
    {
        return current_tick >= last_interaction_tick + REQUIRED_MILLISECONDS_FOR_EXPIRE;
    }

    template <class T>
    bool TConnection<T>::Descriptor::is_safe_delete(std::size_t current_tick) const
    {
        return not online
            && current_tick >= last_offline_tick + REQUIRED_MILLISECONDS_FOR_SECURE_DELETION;
    }

    template <class T>
    void TConnection<T>::Descriptor::set_offline(std::size_t current_tick)
    {
        online = false;
        last_offline_tick = current_tick;
    }

    template <class T>
    bool TConnection<T>::Descriptor::try_interact_with_client()
    {
        if (online) {
            update_last_interaction_time();
            return true;
        }
        return false;
    }

    template <class T>
    void TConnection<T>::Descriptor::activate_receive_cycle(io::IoRecvEvent* event)
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
        if (not client_socket.recv(&event->overlapped, wbuf, 1))
            event->is_processing = false;
    }

    template <class T>
    void TConnection<T>::Descriptor::activate_send_cycle(io::IoSendEvent* event)
    {
        if (event->data.size() == 0)
            return;

        WSABUF wbuf[1] = {};
        wbuf[0].buf = reinterpret_cast<char*>(event->data.begin());
        wbuf[0].len = ULONG(event->data.size());

        // should assign flag first to avoid data race.
        event->is_processing = true; 
        if (not client_socket.send(&event->overlapped, wbuf, 1))
            event->is_processing = false;
    }

    template <class T>
    bool TConnection<T>::Descriptor::disconnect_immediate(std::string_view reason)
    {
        if (not online)
            return false;

        net::PacketDisconnectPlayer disconnect_packet{ reason };
        auto result = disconnect_packet.serialize(io_send_events[SendType::IMMEDIATE]->data);

        set_offline();
        return result;
    }

    template <class T>
    bool TConnection<T>::Descriptor::disconnect_deferred(std::string_view reason)
    {
        if (not online)
            return false;

        net::PacketDisconnectPlayer disconnect_packet{ reason };
        auto result = disconnect_packet.serialize(io_send_events[SendType::DEFERRED]->data);
    
        set_offline();
        return result;
    }

    template <class T>
    bool TConnection<T>::Descriptor::finalize_handshake() const
    {
        if (not online)
            return false;

        const auto& conf = config::get_config();

        PacketHandshake handshake_packet{
            conf.server.server_name, conf.server.motd, 
            self_player->get_player_type() == game::PlayerType::ADMIN ? net::UserType::OP : net::UserType::NORMAL
        };

        return handshake_packet.serialize(io_send_events[SendType::DEFERRED]->data);
    }

    template <class T>
    void TConnection<T>::Descriptor::associate_game_player
        (game::PlayerID player_id, game::PlayerType player_type, const char* username, const char* password)
    {
        self_player = std::make_unique<game::Player>(
            player_id,
            player_type,
            username,
            password
        );
    }

    template class TConnection<net::Socket>;
}