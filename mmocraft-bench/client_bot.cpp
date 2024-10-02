#include "client_bot.h"

#include <atomic>
#include <random>
#include <stdio.h>

#include "net/packet.h"
#include "logging/logger.h"
#include "statistics.h"

#define CONSUME_PACKET_SIZE(packet_size) \
    remain_packet_size = packet_size; \
    remain_packet_size -= std::min(packet_size, std::size_t(data_end - data_begin)); \
    data_begin += packet_size - remain_packet_size; \

namespace
{
    std::atomic<int> client_counter{ 0 };

    std::random_device random_device;
    std::mt19937 random_generator(random_device());
    std::uniform_int_distribution<int> random_distributor;

    int random_int()
    {
        return random_distributor(random_generator);
    }
}

namespace bench
{
    ClientBot::ClientBot(io::IoService& io_service)
        : _id{ client_counter.fetch_add(1, std::memory_order_relaxed) }
        , _sock{ net::create_windows_socket(net::socket_protocol_id::tcp_rio_v4) }
        , last_interactin_at{util::current_monotonic_tick()}
    {
        // Create socket for client
        win::UniqueSocket sock{ _sock };

        {
            BOOL opt = TRUE;
            if (auto result = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt))) {
                CONSOLE_LOG(error) << "SO_REUSEADDR failed: %d\n" << WSAGetLastError();
            }
        }

        // ConnectEx requires the socket to be initially bound.
        struct sockaddr_in addr;
        ZeroMemory(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = 0;

        if (::bind(sock.get(), (SOCKADDR*)&addr, sizeof(addr)))
            CONSOLE_LOG(fatal) << "could't bind initially.";

        connection_io.reset(new net::ConnectionIO(net::ConnectionID(_id), io_service, std::move(sock)));
        connection_io->register_event_handler(this);
    }

    bool ClientBot::connect(std::string_view ip, int port)
    {
        return connection_io->post_connect_event(&io_connect_event, ip, port);
    }

    void ClientBot::disconnect()
    {
        connection_io->close();
    }

    void ClientBot::post_recv_event()
    {
        std::unique_lock lock(network_io_lock);
        connection_io->post_recv_event();
    }

    void ClientBot::post_send_event()
    {
        if (not connection_io->is_send_io_busy()) {
            std::unique_lock lock(network_io_lock);
            connection_io->post_send_event();
        }
    }

    void ClientBot::send_handshake()
    {
        char username[64];
        ::snprintf(username, sizeof(username), "user%d", _id);
        net::PacketHandshake packet(username, "password", 0);
        connection_io->send_packet(packet);

        post_send_event();
    }

    void ClientBot::send_ping()
    {
        if (state() >= ClientState::Connected) {
            net::PacketExtPing ping_packet;
            ping_packet.set_request_time();
            connection_io->send_packet(ping_packet);

            post_send_event();
        }
    }

    void ClientBot::send_random_block(util::Coordinate3D map_size)
    {
        auto pos = util::Coordinate3D{ random_int() % map_size.x,
                                        random_int() % map_size.y,
                                        random_int() % map_size.z };
        auto mode = std::uint8_t(random_int() & 1);
        auto block_id = std::uint8_t(random_int() % 20);

        net::PacketSetBlockClient packet(pos, mode, block_id);
        connection_io->send_packet(packet);

        post_send_event();
    }

    /* IOEvent handlers */

    void ClientBot::on_error()
    {
        _state = ClientState::Error;
    }

    std::size_t ClientBot::handle_io_event(io::IoConnectEvent* event)
    {
        if (auto result = setsockopt(event->connected_socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0)) {
            on_error();
            return 0;
        }

        last_interactin_at = util::current_monotonic_tick();
        event->is_processing = false;
        return 0;
    }

    void ClientBot::on_complete(io::IoConnectEvent* event)
    {
        if (_state != ClientState::Error) {
            _state = ClientState::Connected;
            post_recv_event();
        }

        event->is_processing = false;
    }

    std::size_t ClientBot::handle_io_event(io::IoSendEvent*)
    {
        last_interactin_at = util::current_monotonic_tick();
        return 0;
    }

    void ClientBot::on_complete(io::IoSendEvent* event, std::size_t transferred_bytes)
    {
        event->is_processing = false;

        bench::on_packet_send(transferred_bytes);
    }

    std::size_t ClientBot::handle_io_event(io::IoRecvEvent* event)
    {
        last_interactin_at = util::current_monotonic_tick();

        auto data_begin = event->event_data()->begin();
        const auto data_end = event->event_data()->end();
        const auto data_size = event->event_data()->size();

        // handle partial receive.
        if (auto partial_packet_size = std::min(remain_packet_size, data_size)) {
            remain_packet_size -= partial_packet_size;
            data_begin += partial_packet_size;
        }

        while (remain_packet_size == 0 && data_begin < data_end) {
            auto packet_id = net::packet_type_id::value(data_begin[0]);

            switch (packet_id) {
            case net::packet_type_id::handshake:
            {
                CONSUME_PACKET_SIZE(net::PacketHandshake::packet_size);
                _state = ClientState::Handshake_Completed;
            }
            break;
            case net::packet_type_id::ping:
            {
                CONSUME_PACKET_SIZE(net::PacketPing::packet_size);
            }
            break;
            case net::packet_type_id::level_initialize:
            {
                CONSUME_PACKET_SIZE(net::PacketLevelInit::packet_size);
            }
            break;
            case net::packet_type_id::level_datachunk:
            {
                CONSUME_PACKET_SIZE(net::PacketLevelDataChunk::packet_size);
            }
            break;
            case net::packet_type_id::level_finalize:
            {
                CONSUME_PACKET_SIZE(std::size_t(7));
                _state = ClientState::Level_Initialized;
            }
            break;
            case net::packet_type_id::set_block_server:
            {
                CONSUME_PACKET_SIZE(net::PacketSetBlockServer::packet_size);
            }
            break;
            case net::packet_type_id::spawn_player:
            {
                CONSUME_PACKET_SIZE(net::PacketSpawnPlayer::packet_size);
            }
            break;
            case net::packet_type_id::set_player_position:
            {
                CONSUME_PACKET_SIZE(net::PacketSetPlayerPosition::packet_size);
            }
            break;
            case net::packet_type_id::update_player_position:
            {
                CONSUME_PACKET_SIZE(std::size_t(7));
            }
            break;
            case net::packet_type_id::update_player_coordinate:
            {
                CONSUME_PACKET_SIZE(std::size_t(5));
            }
            break;
            case net::packet_type_id::update_player_orientation:
            {
                CONSUME_PACKET_SIZE(std::size_t(4));
            }
            break;
            case net::packet_type_id::despawn_player:
            {
                CONSUME_PACKET_SIZE(net::PacketDespawnPlayer::packet_size);
            }
            break;
            case net::packet_type_id::chat_message:
            {
                CONSUME_PACKET_SIZE(net::PacketChatMessage::packet_size);
            }
            break;
            case net::packet_type_id::disconnect_player:
            {
                CONSUME_PACKET_SIZE(net::PacketDisconnectPlayer::packet_size);
                _state = ClientState::Error;
            }
            break;
            case net::packet_type_id::set_playerid:
            {
                CONSUME_PACKET_SIZE(net::PacketSetPlayerID::packet_size);
            }
            break;
            case net::packet_type_id::ext_ping:
            {
                on_ping_packet_receive(data_begin);

                CONSUME_PACKET_SIZE(net::PacketExtPing::packet_size);
            }
            break;
            default:
                CONSOLE_LOG(error) << "Unexpected packet id: " << int(packet_id);
            }
        }

        bench::on_packet_receive(data_size);
        return data_size;
    }

    void ClientBot::on_complete(io::IoRecvEvent* event)
    {
        event->is_processing = false;

        if (_state != ClientState::Error)
            post_recv_event();
    }
}