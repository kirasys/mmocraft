#include "pch.h"
#include "game_server.h"

#include <array>
#include <cstring>
#include <filesystem>

#include "config/config.h"
#include "config/constants.h"
#include "logging/error.h"
#include "database/query.h"

namespace
{
    const std::array<net::GameServer::packet_handler_type, 0x100> packet_handler_db = [] {
        std::array<net::GameServer::packet_handler_type, 0x100> arr{};
        arr[net::packet_type_id::handshake] = &net::GameServer::handle_handshake_packet;
        arr[net::packet_type_id::ping] = &net::GameServer::handle_ping_packet;
        arr[net::packet_type_id::set_block_client] = &net::GameServer::handle_set_block_packet;
        arr[net::packet_type_id::set_player_position] = &net::GameServer::handle_player_position_packet;
        arr[net::packet_type_id::chat_message] = &net::GameServer::handle_chat_message_packet;
        arr[net::packet_type_id::ext_info] = &net::GameServer::handle_ext_info_packet;
        arr[net::packet_type_id::ext_entry] = &net::GameServer::handle_ext_entry_packet;
        arr[net::packet_type_id::two_way_ping] = &net::GameServer::handle_two_way_ping_packet;
        arr[net::packet_type_id::ext_ping] = &net::GameServer::handle_ext_ping_packet;
        return arr;
    }();

    std::array<io::AsyncTask<void>(net::GameServer::*)(net::MessageRequest&), 0x100> message_handler_table = [] {
        std::array<io::AsyncTask<void>(net::GameServer::*)(net::MessageRequest&), 0x100> arr{};
        arr[net::message_id::packet_handshake] = &net::GameServer::handle_handshake_response_message;
        return arr;
    }();
}

namespace net
{
    GameServer::GameServer(unsigned max_clients, int num_of_event_threads)
        : connection_env{ max_clients }
        , io_service { max_clients, num_of_event_threads }
        , tcp_server{ *this, connection_env, io_service }
        , udp_server{ *this }

        , world{ connection_env }
        
        , interval_tasks{ this }
    {
        interval_tasks.schedule(util::interval_task_tag_id::announce_server,
            &GameServer::announce_server,
            util::MilliSecond(config::task::announce_server_period)
        );
    }

    error::ResultCode GameServer::handle_packet(net::Connection& conn, const std::byte* packet_data)
    {
        auto [packet_id, packet_size] = PacketStructure::parse_packet(packet_data);

        if (auto handler = packet_handler_db[packet_id])
            return (this->*handler)(conn, { packet_data, std::size_t(packet_size) });

        CONSOLE_LOG(error) << "Unimplemented packetd id : " << packet_id;
        return error::code::packet::unimplemented_packet_id;
    }

    error::ResultCode GameServer::handle_handshake_packet(net::Connection& conn, util::byte_view packet_data)
    {
        net::PacketHandshake packet(packet_data.data());

        conn.set_player(std::make_unique<game::Player>(
            conn.connection_key(),
            packet.username
        ));

        if (auto player = conn.associated_player()) {
            if (packet.cpe_magic == 0x42) // is CPE supported
                player->prepare_state_transition(game::PlayerState::ex_handshaking, game::PlayerState::ex_handshaked);
            else
                player->prepare_state_transition(game::PlayerState::handshaking, game::PlayerState::handshaked);
        }

        udp_server.communicator().forward_packet(
            protocol::server_type_id::login,
            net::message_id::packet_handshake,
            conn.connection_key(),
            packet_data
        );

        return error::code::success;
    }

    error::ResultCode GameServer::handle_ping_packet(net::Connection& conn, util::byte_view packet_data)
    {
        // send pong.
        conn.io()->send_ping();
        return error::code::success;
    }

    error::ResultCode GameServer::handle_two_way_ping_packet(net::Connection& conn, util::byte_view packet_data)
    {
        conn.io()->send_raw_data(packet_data.data(), packet_data.size());
        return error::code::success;
    }

    error::ResultCode GameServer::handle_ext_ping_packet(net::Connection& conn, util::byte_view packet_data)
    {
        net::PacketExtPing packet(packet_data.data());
        conn.io()->send_packet(packet);

        return error::code::success;
    }

    error::ResultCode GameServer::handle_set_block_packet(net::Connection& conn, util::byte_view packet_data)
    {
        net::PacketSetBlockClient packet(packet_data.data());

        auto block_id = packet.block_creation_mode() == game::block_creation_mode::set ? packet.block_id : game::block_id::air;
        if (not world.try_change_block({ packet.x, packet.y, packet.z }, block_id))
            goto REVERT_BLOCK;

        return error::code::success;

    REVERT_BLOCK:
        block_id = packet.block_creation_mode() == game::block_creation_mode::set ? game::block_id::air : packet.block_id;
        net::PacketSetBlockServer revert_block_packet({ packet.x, packet.y, packet.z }, block_id);

        conn.io()->send_packet(revert_block_packet);

        return error::code::success;
    }

    error::ResultCode GameServer::handle_player_position_packet(net::Connection& conn, util::byte_view packet_data)
    {
        net::PacketSetPlayerPosition packet(packet_data.data());

        if (auto player = conn.associated_player())
            player->set_position(packet.player_pos);
        
        return error::code::success;
    }

    error::ResultCode GameServer::handle_chat_message_packet(net::Connection& conn, util::byte_view packet_data)
    {
        net::PacketChatMessage packet(packet_data.data());

        if (packet.message[0] != '/') // if common chat
            world.try_add_common_chat(packet_data);

        // send chat message to the login server.
        protocol::ChatCommandRequest chat_command_msg;
        chat_command_msg.mutable_message()->append(packet.message);
        if (auto player = conn.associated_player())
            chat_command_msg.mutable_sender_player_name()->append(player->username());

        net::MessageRequest request(net::message_id::chat_command, chat_command_msg);
        udp_server.communicator().send_to(request, protocol::server_type_id::chat);

        return error::code::success;
    }

    error::ResultCode GameServer::handle_ext_info_packet(net::Connection& conn, util::byte_view packet_data)
    {
        net::PacketExtInfo packet(packet_data.data());

        if (auto player = conn.associated_player()) {
            player->set_extension_count(packet.extension_count);
        }

        return error::code::success;
    }

    error::ResultCode GameServer::handle_ext_entry_packet(net::Connection& conn, util::byte_view packet_data)
    {
        net::PacketExtEntry packet(packet_data.data());

        if (auto player = conn.associated_player()) {
            if (net::is_cpe_support(packet.extenstion_name, packet.version))
                player->register_extension(net::cpe_index_of(packet.extenstion_name));

            if (player->decrease_pending_extension_count() == 0)
                player->transit_state(game::PlayerState::extension_synced);
        }

        return error::code::success;
    }

    /**
     *  Message handlers.
     */

    bool GameServer::handle_message(net::MessageRequest& request)
    {
        if (auto handler = message_handler_table[request.message_id()]) {
            (this->*handler)(request).start();
            return true;
        }

        return false;
    }

    io::AsyncTask<void> GameServer::handle_handshake_response_message(MessageRequest& request)
    {
        protocol::PacketHandshakeResponse msg;
        if (not request.parse_message(msg))
            co_return;

        auto connection_key = msg.connection_key();

        if (auto conn = connection_env.try_acquire_connection(connection_key)) {
            auto player = conn->associated_player();

            if (msg.error_code() == error::code::packet::player_not_exist) {
                player->set_player_type(game::player_type_id::guest);
                player->transit_state();
                co_return;
            }
            else if (msg.error_code() != error::code::success) {
                conn->kick(error::code::packet::player_login_fail);
                co_return;
            }

            player->set_player_type(game::player_type_id(msg.player_type()));
            player->set_uuid(msg.player_uuid());

            // Disconnect already logged in player.
            if (auto prev_conn = connection_env.try_acquire_connection(msg.prev_connection_key()))
                prev_conn->kick(error::code::packet::player_already_login);
        }

        // Load player game data.
        auto [err, result] = co_await ::database::CouchbaseCore::get_document(::database::CollectionPath::player_gamedata, msg.player_uuid());

        if (auto conn = connection_env.try_acquire_connection(connection_key)) {
            if (err && err.ec() != couchbase::errc::key_value::document_not_found) {
                conn->kick(error::code::packet::player_login_fail);
                co_return;
            }

            auto player = conn->associated_player();

            if (err.ec() != couchbase::errc::key_value::document_not_found) {
                auto gamedata = result.content_as<::database::collection::PlayerGamedata>();
                player->set_gamedata(gamedata);
            }

            player->transit_state();
        }
    }

    void GameServer::tick()
    {
        ConnectionIO::flush_send(connection_env);
        ConnectionIO::flush_receive(connection_env);

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
        comm.register_server(protocol::server_type_id::router, { router_ip, router_port });

        auto& conf = config::get_config();

        // Fetch other UDP server.
        comm.fetch_server_address(protocol::server_type_id::login);
        comm.fetch_server_address(protocol::server_type_id::chat);

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

    void GameServer::on_disconnect(net::Connection& conn)
    {
        if (auto player = conn.associated_player()) {
            // notify logout event to the login server.
            if (player->state() >= game::PlayerState::handshaked) {
                protocol::PlayerLogoutRequest logout_msg;
                logout_msg.mutable_username()->append(player->username());

                net::MessageRequest request(net::message_id::player_logout, logout_msg);
                udp_server.communicator().send_to(request, protocol::server_type_id::login);
            }
        }
    }
}