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
                                io::IoService& io_service)
        : packet_handle_server{ a_packet_handle_server }
        , _connection_key{ a_connection_key }
        , connection_env{ a_connection_env }
        , online{ true }
        , connection_io { new ConnectionIO(std::move(sock)) }
    {
        if (not is_valid())
            throw error::ErrorCode::CLIENT_CONNECTION_CREATE;

        // start to service client socket events.
        connection_io->register_event_handler(io_service, this);
        connection_io->emit_receive_event();

        update_last_interaction_time();
    }

    Connection::~Connection()
    {
        connection_env.on_connection_delete(_connection_key);
    }

    void Connection::set_offline(std::size_t current_tick)
    {
        online = false;
        last_offline_tick = current_tick;
        connection_env.on_connection_offline(_connection_key);
    }

    bool Connection::is_expired(std::size_t current_tick) const
    {
        return current_tick >= last_interaction_tick + REQUIRED_MILLISECONDS_FOR_EXPIRE;
    }

    bool Connection::is_safe_delete(std::size_t current_tick) const
    {
        return not online
            && current_tick >= last_offline_tick + REQUIRED_MILLISECONDS_FOR_SECURE_DELETION;
    }

    bool Connection::try_interact_with_client()
    {
        if (online) {
            update_last_interaction_time();
            return true;
        }
        return false;
    }

    void Connection::disconnect()
    {
        // player manager(aka. world) has some extra works for disconnecting players.
        // in this case, return without setting offline the connection.
        if (_player && _player->state() >= game::PlayerState::Spawned) {
            _player->set_state(game::PlayerState::Disconnect_Wait);
            return;
        }

        set_offline();
    }

    void Connection::disconnect_with_message(std::string_view message)
    {
        connection_io->send_disconnect_message(message);
        disconnect();
    }

    void Connection::disconnect_with_message(error::ResultCode result)
    {
        disconnect_with_message(result.to_string());
    }

    std::size_t Connection::process_packets(std::byte* data_begin, std::byte* data_end)
    {
        char packet_buf[256]; // Note: client packet structure must not be more than 256 bytes.
        auto packet_ptr = reinterpret_cast<Packet*>(packet_buf);
        auto data_cur = data_begin;

        while (data_cur < data_end) {
            auto [parsed_bytes, parsing_result] = PacketStructure::parse_packet(data_cur, data_end, packet_ptr);
            if (not parsing_result.is_success()) {
                last_error_code = parsing_result;
                break;
            }

            auto handle_result = packet_handle_server.handle_packet(*this, packet_ptr);
            if (not handle_result.is_packet_handle_success()) {
                last_error_code = handle_result;
                break;
            }

            data_cur += parsed_bytes;
        }

        assert(data_cur <= data_end && "Parsing error");
        return data_cur - data_begin; // num of total parsed bytes.
    }

    void Connection::on_handshake_success()
    {
        assert(_player != nullptr);
        const auto& server_conf = config::get_server_config();

        net::PacketHandshake handshake_packet{
            server_conf.server_name(), server_conf.motd(),
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
        if (last_error_code.is_packet_handle_success()) {
            last_error_code.reset();
            connection_io->emit_receive_event(event);
            return;
        }

        disconnect_with_message(last_error_code.to_string());
        event->is_processing = false;
    }

    std::size_t Connection::handle_io_event(io::IoRecvEvent* event)
    {
        if (not try_interact_with_client()) // timeout case: connection will be deleted soon.
            return 0; 

        return process_packets(event->data->begin(), event->data->end());
    }

    /// send event handler

    void Connection::on_complete(io::IoSendEvent* event, std::size_t transferred_bytes)
    {
        // Note: Can't start I/O event again due to the interleaving problem.
        event->is_processing = false;
    }

    /**
     *  Connection descriptor interface
     */

    ConnectionIO::ConnectionIO(win::UniqueSocket&& a_sock)
        : client_socket{ std::move(a_sock) }
        , io_multicast_send_events(num_of_multicast_event)
    {

    }

    ConnectionIO::~ConnectionIO()
    {

    }

    void ConnectionIO::register_event_handler(io::IoService& io_service, io::IoEventHandler* event_handler)
    {
        io_service.register_event_source(client_socket.get_handle(), event_handler);
    }

    void ConnectionIO::close()
    {
        client_socket.close();
    }

    bool ConnectionIO::emit_connect_event(io::IoAcceptEvent* event, std::string_view ip, int port)
    {
        return client_socket.connect(ip, port, &event->overlapped);
    }

    void ConnectionIO::emit_receive_event()
    {
        emit_receive_event(&io_recv_event);
    }

    void ConnectionIO::emit_receive_event(io::IoRecvEvent* event)
    {
        if (event->data->unused_size() < PacketStructure::max_size_of_packet_struct()) {
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

    void ConnectionIO::emit_send_event(io::IoSendEvent* event)
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

    void ConnectionIO::emit_send_event()
    {
        emit_send_event(&io_send_event);
    }

    bool ConnectionIO::emit_multicast_send_event(io::IoSendEventSharedData* event_data)
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

    bool ConnectionIO::send_raw_data(const std::byte* data, std::size_t data_size) const
    {
        return io_send_event.data->push(data, data_size);
    }

    bool ConnectionIO::send_disconnect_message(std::string_view reason)
    {
        net::PacketDisconnectPlayer disconnect_packet{ reason };
        bool result = disconnect_packet.serialize(*io_send_event.data);

        emit_send_event(&io_send_event);

        return result;
    }

    bool ConnectionIO::send_ping() const
    {
        auto packet = std::byte(net::PacketID::Ping);
        return io_send_event.data->push(&packet, net::PacketPing::packet_size);
    }

    void ConnectionIO::flush_send(net::ConnectionEnvironment& connection_env)
    {
        auto flush_message = [](Connection& conn) {
            // flush immedidate and deferred messages.
            auto connection_io = conn.io();

            if (not connection_io->is_send_io_busy()) {
                connection_io->emit_send_event(&connection_io->io_send_event);
            }
        };

        connection_env.for_each_connection(flush_message);
    }

    void ConnectionIO::flush_receive(net::ConnectionEnvironment& connection_env)
    {
        auto flush_message = [](Connection& conn) {
            auto connection_io = conn.io();

            if (not connection_io->is_receive_io_busy()) {
                connection_io->io_recv_event.is_processing = true;

                // Note: receive event may stop only for one reason: insuffient buffer space.
                //       unlike flush_server_message(), it need to invoke the I/O handler to process pending packets.
                connection_io->io_recv_event.invoke_handler(conn,
                    connection_io->io_recv_event.data->size() ? io::RETRY_SIGNAL : io::EOF_SIGNAL,
                    ERROR_SUCCESS);
            }
        };

        connection_env.for_each_connection(flush_message);
    }
}