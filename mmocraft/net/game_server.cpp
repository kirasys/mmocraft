#include "pch.h"
#include "game_server.h"

#include <array>
#include <cstring>
#include <filesystem>

#include "config/config.h"
#include "logging/error.h"
#include "database/query.h"
#include "game/player_command.h"

namespace
{
    const std::array<net::GameServer::packet_handler_type, 0x100> packet_handler_db = [] {
        std::array<net::GameServer::packet_handler_type, 0x100> arr{};
        arr[net::PacketID::Handshake] = &net::GameServer::handle_handshake_packet;
        arr[net::PacketID::Ping] = &net::GameServer::handle_ping_packet;
        arr[net::PacketID::SetBlockClient] = &net::GameServer::handle_set_block_packet;
        arr[net::PacketID::SetPlayerPosition] = &net::GameServer::handle_player_position_packet;
        arr[net::PacketID::ChatMessage] = &net::GameServer::handle_chat_message_packet;
        arr[net::PacketID::ExtInfo] = &net::GameServer::handle_ext_info_packet;
        arr[net::PacketID::ExtEntry] = &net::GameServer::handle_ext_entry_packet;
        arr[net::PacketID::TwoWayPing] = &net::GameServer::handle_two_way_ping_packet;
        arr[net::PacketID::ExtPing] = &net::GameServer::handle_ext_ping_packet;
        return arr;
    }();

    std::array<net::GameServer::message_handler_type, 0x100> message_handler_table = [] {
        std::array<net::GameServer::message_handler_type, 0x100> arr{};
        arr[net::MessageID::Login_PacketHandshake] = &net::GameServer::handle_handshake_response_message;
        return arr;
    }();
}

namespace net
{
    GameServer::GameServer(unsigned max_clients, int num_of_event_threads)
        : connection_env{ max_clients }
        , io_service { max_clients, num_of_event_threads }
        , tcp_server{ *this, connection_env, io_service }
        , udp_server{ this, &message_handler_table }

        , world{ connection_env }
        
        , deferred_chat_message_packet_task{ &GameServer::handle_deferred_chat_message_packet, this, chat_message_task_interval }

        , interval_tasks{ this }
    {
        interval_tasks.schedule(util::TaskTag::ANNOUNCE_SERVER, 
            &GameServer::announce_server,
            util::MilliSecond(config::announce_server_period_ms)
        );
    }

    error::ResultCode GameServer::handle_packet(net::Connection& conn, const std::byte* packet_data)
    {
        auto [packet_id, packet_size] = PacketStructure::parse_packet(packet_data);

        if (auto handler = packet_handler_db[packet_id])
            return (this->*handler)(conn, packet_data, std::size_t(packet_size));

        CONSOLE_LOG(error) << "Unimplemented packetd id : " << packet_id;
        return error::PACKET_UNIMPLEMENTED_ID;
    }

    error::ResultCode GameServer::handle_handshake_packet(net::Connection& conn, const std::byte* data, std::size_t data_size)
    {
        net::PacketHandshake packet(data);

        conn.set_player(std::make_unique<game::Player>(
            conn.connection_key(),
            packet.username
        ));

        if (auto player = conn.associated_player()) {
            if (packet.cpe_magic == 0x42) // is CPE supported
                player->set_extension_mode();

            player->set_state(game::PlayerState::Handshake_Wait);
        }

        udp_server.communicator().forward_packet(
            protocol::ServerType::Login,
            net::MessageID::Login_PacketHandshake,
            conn.connection_key(),
            data, data_size
        );

        return error::PACKET_HANDLE_DEFERRED;
    }

    error::ResultCode GameServer::handle_ping_packet(net::Connection& conn, const std::byte* data, std::size_t data_size)
    {
        // send pong.
        conn.io()->send_ping();
        return error::SUCCESS;
    }

    error::ResultCode GameServer::handle_two_way_ping_packet(net::Connection& conn, const std::byte* data, std::size_t data_size)
    {
        conn.io()->send_raw_data(data, data_size);
        return error::SUCCESS;
    }

    error::ResultCode GameServer::handle_ext_ping_packet(net::Connection& conn, const std::byte* data, std::size_t data_size)
    {
        net::PacketExtPing packet(data);
        conn.io()->send_packet(packet);

        return error::SUCCESS;
    }

    error::ResultCode GameServer::handle_set_block_packet(net::Connection& conn, const std::byte* data, std::size_t data_size)
    {
        net::PacketSetBlockClient packet(data);

        auto block_id = packet.mode == game::BlockMode::SET ? packet.block_id : game::BLOCK_AIR;
        if (not world.try_change_block({ packet.x, packet.y, packet.z }, block_id))
            goto REVERT_BLOCK;

        return error::SUCCESS;

    REVERT_BLOCK:
        block_id = packet.mode == game::BlockMode::SET ? game::BLOCK_AIR : packet.block_id;
        net::PacketSetBlockServer revert_block_packet({ packet.x, packet.y, packet.z }, block_id);

        conn.io()->send_packet(revert_block_packet);

        return error::SUCCESS;
    }

    error::ResultCode GameServer::handle_player_position_packet(net::Connection& conn, const std::byte* data, std::size_t data_size)
    {
        net::PacketSetPlayerPosition packet(data);

        if (auto player = conn.associated_player())
            player->set_position(packet.player_pos);
        
        return error::SUCCESS;
    }

    error::ResultCode GameServer::handle_chat_message_packet(net::Connection& conn, const std::byte* data, std::size_t data_size)
    {
        net::PacketChatMessage packet(data);

        if (auto player = conn.associated_player()) {
            packet.player_id = player->game_id(); // client always sends 0xff(SELF ID).

            if (packet.message[0] == '/') { // if command message
                game::PlayerCommand command(player);
                command.execute(world, packet.message);

                net::PacketChatMessage msg_packet(command.get_response());
                conn.io()->send_packet(msg_packet);
;            }
            else {
                deferred_chat_message_packet_task.push_packet(conn.connection_key(), packet);
                return error::PACKET_HANDLE_DEFERRED;
            }
        }
        return error::SUCCESS;
    }

    error::ResultCode GameServer::handle_ext_info_packet(net::Connection& conn, const std::byte* data, std::size_t data_size)
    {
        net::PacketExtInfo packet(data);

        if (auto player = conn.associated_player()) {
            player->set_extension_count(packet.extension_count);
        }

        return error::SUCCESS;
    }

    error::ResultCode GameServer::handle_ext_entry_packet(net::Connection& conn, const std::byte* data, std::size_t data_size)
    {
        net::PacketExtEntry packet(data);

        if (auto player = conn.associated_player()) {
            if (net::is_cpe_support(packet.extenstion_name, packet.version))
                player->register_extension(net::cpe_index_of(packet.extenstion_name));

            if (player->decrease_pending_extension_count() == 0)
                conn.on_handshake_success();
        }

        return error::SUCCESS;
    }

    void GameServer::tick()
    {
        ConnectionIO::flush_send(connection_env);
        ConnectionIO::flush_receive(connection_env);

        flush_deferred_packet();

        if (tcp_server.is_stopped())
            tcp_server.start_accept();
    }

    void GameServer::announce_server()
    {
        auto& conf = config::get_config();

        if (not udp_server.communicator().announce_server(server_type, {
            .ip = conf.udp_server().ip(),
            .port = conf.udp_server().port()
            })) {
            CONSOLE_LOG(error) << "Fail to announce game server";
        }
    }

    bool GameServer::initialize(const char* router_ip, int router_port)
    {
        auto& comm = udp_server.communicator();
        comm.register_server(protocol::ServerType::Router, { router_ip, router_port });

        auto& conf = config::get_config();

        // Fetch other UDP server.
        comm.fetch_server(protocol::ServerType::Login);
        comm.fetch_server(protocol::ServerType::Chat);

        // Create working directories
        if (not std::filesystem::exists(conf.world().save_dir()))
            std::filesystem::create_directories(conf.world().save_dir());

        return true;
    }

    void GameServer::serve_forever(const char* router_ip, int router_port)
    {
        if (not initialize(router_ip, router_port))
            return;

        // start UDP server.
        const auto& conf = config::get_config();
        udp_server.start_network_io_service(conf.udp_server().ip(), conf.udp_server().port(), 1);

        // start network I/O system.
        tcp_server.start_network_io_service(conf.tcp_server().ip(), conf.tcp_server().port(), conf.system().num_of_processors() * 2);

        // load world map.
        world.load_filesystem_world(conf.world().save_dir());

        while (1) {
            std::size_t start_tick = util::current_monotonic_tick();

            interval_tasks.process_tasks();

            this->tick();

            world.tick(io_service);

            std::size_t end_tick = util::current_monotonic_tick();

            if (auto diff = end_tick - start_tick; diff < 100)
                util::sleep_ms(std::max(100 - diff, std::size_t(30)));
        }
    }

    void GameServer::flush_deferred_packet()
    {
        for (auto task : deferred_packet_tasks) {
            if (task->ready()) {
                io_service.schedule_task(task);
            }
        }
    }

    void GameServer::on_disconnect(net::Connection& conn)
    {
        if (auto player = conn.associated_player()) {
            // notify logout event to the login server.
            if (player->state() >= game::PlayerState::Handshake_Completed) {
                protocol::PlayerLogoutRequest logout_request;
                logout_request.mutable_username()->append(player->username());
                udp_server.communicator().send_message(protocol::ServerType::Login, net::MessageID::Login_PlayerLogout, logout_request);
            }
        }
    }

    /**
     *  Message handlers.
     */

    bool GameServer::handle_handshake_response_message(const MessageRequest& request, MessageResponse& response)
    {
        protocol::PacketHandshakeResponse msg;
        if (not msg.ParseFromArray(request.begin_message(), int(request.message_size())))
            return false;

        if (auto conn = connection_env.try_acquire_connection(msg.connection_key())) {
            auto& player = *conn->associated_player();

            switch (msg.error_code()) {
            case error::PACKET_HANDLE_SUCCESS:
            {
                // Disconnect already logged in player.
                if (msg.connection_key() != msg.prev_connection_key())
                    if (auto prev_conn = connection_env.try_acquire_connection(msg.prev_connection_key()))
                        prev_conn->kick(error::PACKET_RESULT_ALREADY_LOGIN);

                player.set_uuid(msg.player_uuid());
                player.set_player_type(game::PlayerType(msg.player_type()));

                // Load player game data.
                database::PlayerGamedata::load(connection_env, player);
                break;
            }
            case error::PACKET_RESULT_NOT_EXIST_LOGIN:
                player.set_player_type(game::PlayerType::GUEST);
                player.set_state(game::PlayerState::ExHandshake_Completed); // todo: state transition
                break;
            default:
                conn->kick(error::PACKET_RESULT_FAIL_LOGIN);
                return true;
            }
        }

        return true;
    }

    /**
     *  Deferred packet handlers.
     */

    void GameServer::handle_deferred_chat_message_packet(io::Task* task, const DeferredPacket<net::PacketChatMessage>* packet_head)
    {
        std::vector<const DeferredPacket<net::PacketChatMessage>*> packets;
        for (auto packet = packet_head; packet; packet = packet->next)
            packets.push_back(packet);

        std::unique_ptr<std::byte[]> packet_data;
        if (auto data_size = DeferredPacket<net::PacketChatMessage>::serialize(packets, packet_data)) {
            //world.multicast_to_world_player(net::MuticastTag::Chat_Message, std::move(packet_data), data_size);
        }
    }
}