#include "pch.h"
#include "master_server.h"

#include <array>
#include <cstring>
#include <filesystem>

#include "config/config.h"
#include "logging/error.h"
#include "database/query.h"
#include "game/player_command.h"

namespace
{
    const std::array<net::MasterServer::handler_type, 0x100> packet_handler_db = [] {
        std::array<net::MasterServer::handler_type, 0x100> arr{};
        arr[net::PacketID::Handshake] = &net::MasterServer::handle_handshake_packet;
        arr[net::PacketID::Ping] = &net::MasterServer::handle_ping_packet;
        arr[net::PacketID::SetBlockClient] = &net::MasterServer::handle_set_block_packet;
        arr[net::PacketID::SetPlayerPosition] = &net::MasterServer::handle_player_position_packet;
        arr[net::PacketID::ChatMessage] = &net::MasterServer::handle_chat_message_packet;
        arr[net::PacketID::ExtInfo] = &net::MasterServer::handle_ext_info_packet;
        arr[net::PacketID::ExtEntry] = &net::MasterServer::handle_ext_entry_packet;
        return arr;
        }();
}

namespace net
{
    MasterServer::MasterServer(net::ConnectionEnvironment& a_connection_env, io::IoCompletionPort& a_io_service)
        : connection_env{ a_connection_env }
        , io_service { a_io_service }
        , tcp_server{ *this, connection_env, io_service }
        , udp_server{ this, nullptr, nullptr }

        , world{ connection_env, database_core }
        
        , deferred_handshake_packet_task{ &MasterServer::handle_deferred_handshake_packet, this, user_authentication_task_interval }
        , deferred_chat_message_packet_task{ &MasterServer::handle_deferred_chat_message_packet, this, chat_message_task_interval }
    {

    }

    error::ResultCode MasterServer::handle_packet(net::Connection& conn, const std::byte* packet_data)
    {
        auto [packet_id, packet_size] = PacketStructure::parse_packet(packet_data);

        if (auto handler = packet_handler_db[packet_id])
            return (this->*handler)(conn, packet_data, std::size_t(packet_size));

        CONSOLE_LOG(error) << "Unimplemented packetd id : " << packet_id;
        return error::PACKET_UNIMPLEMENTED_ID;
    }

    error::ResultCode MasterServer::handle_handshake_packet(net::Connection& conn, const std::byte* data, std::size_t data_size)
    {
        net::PacketHandshake packet(data);

        //deferred_handshake_packet_task.push_packet(conn.connection_key(), packet);
        udp_server.communicator().forward_packet(protocol::ServerType::Login, conn.connection_key(), data, data_size);

        return error::PACKET_HANDLE_DEFERRED;
    }

    error::ResultCode MasterServer::handle_ping_packet(net::Connection& conn, const std::byte* data, std::size_t data_size)
    {
        // send pong.
        conn.io()->send_ping();
        return error::SUCCESS;
    }

    error::ResultCode MasterServer::handle_set_block_packet(net::Connection& conn, const std::byte* data, std::size_t data_size)
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

    error::ResultCode MasterServer::handle_player_position_packet(net::Connection& conn, const std::byte* data, std::size_t data_size)
    {
        net::PacketSetPlayerPosition packet(data);

        if (auto player = conn.associated_player())
            player->set_position(packet.player_pos);
        
        return error::SUCCESS;
    }

    error::ResultCode MasterServer::handle_chat_message_packet(net::Connection& conn, const std::byte* data, std::size_t data_size)
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

    error::ResultCode MasterServer::handle_ext_info_packet(net::Connection& conn, const std::byte* data, std::size_t data_size)
    {
        net::PacketExtInfo packet(data);

        if (auto player = conn.associated_player()) {
            player->set_extension_count(packet.extension_count);
        }

        return error::SUCCESS;
    }

    error::ResultCode MasterServer::handle_ext_entry_packet(net::Connection& conn, const std::byte* data, std::size_t data_size)
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

    void MasterServer::tick()
    {
        ConnectionIO::flush_send(connection_env);
        ConnectionIO::flush_receive(connection_env);

        flush_deferred_packet();

        if (tcp_server.is_stopped())
            tcp_server.start_accept();
    }

    bool MasterServer::initialize(const char* router_ip, int router_port)
    {
        auto& comm = udp_server.communicator();
        comm.register_server(protocol::ServerType::Router, { router_ip, router_port });

        // Fetch other UDP server.
        comm.fetch_server(protocol::ServerType::Login);
        comm.fetch_server(protocol::ServerType::Chat);

        if (not comm.load_remote_config(server_type, config::get_config()))
            return false;

        auto& conf = config::get_config();
        config::set_default_configuration(*conf.mutable_system());

        logging::initialize_system(conf.log().log_dir(), conf.log().log_filename());

        database::initialize_system();

        // Create working directories
        if (not std::filesystem::exists(conf.world().save_dir()))
            std::filesystem::create_directories(conf.world().save_dir());

        return true;
    }

    void MasterServer::serve_forever(const char* router_ip, int router_port)
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

            this->tick();
            world.tick(io_service);

            std::size_t end_tick = util::current_monotonic_tick();

            if (auto diff = end_tick - start_tick; diff < 100)
                util::sleep_ms(std::max(100 - diff, std::size_t(30)));
        }
    }

    void MasterServer::flush_deferred_packet()
    {
        for (auto task : deferred_packet_tasks) {
            if (task->ready()) {
                io_service.schedule_task(task);
            }
        }
    }

    /**
     *  Deferred packet handler methods.
     */

    void MasterServer::handle_deferred_handshake_packet(io::Task* task, const DeferredPacket<net::PacketHandshake>* packet_head)
    {
        database::PlayerLoginSQL player_login;

        if (not player_login.is_valid()) {
            CONSOLE_LOG(error) << "Fail to allocate sql statement handles.";
            return;
        }

        for (auto packet = packet_head; packet; packet = packet->next) {
            player_login.authenticate(packet->username, packet->password);

            // return handshake result to clients.

            if (auto conn = connection_env.try_acquire_connection(packet->connection_key)) {
                if (player_login.player_type() == game::PlayerType::INVALID) {
                    conn->disconnect_with_message(error::PACKET_RESULT_FAIL_LOGIN);
                    continue;
                }

                if (world.is_already_exist_player(packet->username)) {
                    conn->disconnect_with_message(error::PACKET_RESULT_ALREADY_LOGIN);
                    continue;
                }

                // Associate player with connection and load game data from database.
                conn->associate_player(std::make_unique<game::Player>(
                    packet->connection_key,
                    player_login.player_identity(),
                    player_login.player_type(),
                    packet->username
                ));

                conn->associated_player()->load_gamedata(
                    player_login.player_gamedata()
                );

                // Register to world player table.
                world.register_player(packet->username, packet->connection_key);

                if (packet->cpe_support)
                    conn->send_supported_cpe_list();
                else
                    conn->on_handshake_success();
            }
        }
    }

    void MasterServer::handle_deferred_chat_message_packet(io::Task* task, const DeferredPacket<net::PacketChatMessage>* packet_head)
    {
        std::vector<const DeferredPacket<net::PacketChatMessage>*> packets;
        for (auto packet = packet_head; packet; packet = packet->next)
            packets.push_back(packet);

        std::unique_ptr<std::byte[]> packet_data;
        if (auto data_size = DeferredPacket<net::PacketChatMessage>::serialize(packets, packet_data)) {
            world.multicast_to_world_player(net::MuticastTag::Chat_Message, std::move(packet_data), data_size);
        }
    }
}