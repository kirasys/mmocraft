#include "pch.h"
#include "master_server.h"

#include <array>
#include <cstring>

#include "config/config.h"
#include "logging/error.h"
#include "database/query.h"

namespace net
{
    MasterServer::MasterServer(const config::Configuration& conf)
        : connection_env{ conf.server.max_player }
        , connection_env_task{ &connection_env }
        , server_core{ *this, connection_env, conf }
        , database_core{ }
        
        , deferred_handshake_packet_event{ 
            &MasterServer::handle_deferred_handshake_packet, &MasterServer::handle_deferred_handshake_packet_result
        }
    {
        /// Schedule interval tasks.

        connection_env_task.schedule(
            util::TaskTag::CLEAN_CONNECTION,
            &ConnectionEnvironment::cleanup_expired_connection,
            util::MilliSecond(10000));
    }

    error::ResultCode MasterServer::handle_packet(net::Connection::Descriptor& conn_descriptor, Packet* packet)
    {
        switch (packet->id) {
        case PacketID::Handshake:
            return handle_handshake_packet(conn_descriptor, *static_cast<PacketHandshake*>(packet));
        default:
            return error::PACKET_UNIMPLEMENTED_ID;
        }
    }

    error::ResultCode MasterServer::handle_handshake_packet(net::Connection::Descriptor& conn_descriptor, PacketHandshake& packet)
    {
        deferred_handshake_packet_event.push_packet(&conn_descriptor, packet);
        return error::PACKET_HANDLE_DEFERRED;
    }

    void MasterServer::tick()
    {
        connection_env_task.process_task(util::TaskTag::CLEAN_CONNECTION);
        connection_env.register_pending_connections();
        connection_env.flush_server_message();
        connection_env.flush_client_message();

        flush_deferred_packet();
        handle_deferred_packet_result();
    }

    void MasterServer::serve_forever()
    {
        const auto& conf = config::get_config();

        // start database system.
        if (not database_core.connect_with_password(conf.db))
            throw error::DATABASE_CONNECT;

        // start network I/O system.
        server_core.start_network_io_service();

        while (1) {
            std::size_t start_tick = util::current_monotonic_tick();

            this->tick();

            std::size_t end_tick = util::current_monotonic_tick();

            if (auto diff = end_tick - start_tick; diff < 1000)
                util::sleep_ms(std::max(1000 - diff, std::size_t(100)));
        }
    }

    void MasterServer::flush_deferred_packet()
    {
        for (auto event : deferred_packet_events) {
            if (event->is_exist_pending_packet()
                && event->transit_state(PacketEvent::Unused, PacketEvent::Processing)) {
                server_core.post_event(event, ULONG_PTR(this));
            }
        }
    }

    /**
     *  Deferred packet handler methods.
     */

    void MasterServer::handle_deferred_packet_result()
    {
        for (auto event : deferred_packet_events)
            event->invoke_result_handler(/*handler_instance=*/ this);
    }

    void MasterServer::handle_deferred_handshake_packet_result(const DeferredPacketResult* results)
    {
        for (auto result = results; result; result = result->next) {
            auto result_code = result->result_code;

            if (result_code.is_login_success()) {
                result->connection_descriptor->finalize_handshake();
                continue;
            }

            result->connection_descriptor->disconnect_deferred(
                result_code.to_string()
            );
        }
    }

    void MasterServer::handle_deferred_handshake_packet(net::PacketEvent* event, const DeferredPacket<PacketHandshake>* packet_head)
    {
        database::PlayerLoginSQL player_login{ database_core.get_connection_handle() };
        database::PlayerSearchSQL player_search{ database_core.get_connection_handle() };

        for (auto packet = packet_head; packet; packet = packet->next) {
            auto player_type = game::PlayerType::INVALID;

            if (not player_search.search(packet->username)) {
                player_type = std::strlen(packet->password) ? game::PlayerType::NEW_USER : game::PlayerType::GUEST;
            }
            else if (player_login.authenticate(packet->username, packet->password)) {
                player_type = std::strcmp(packet->username, "admin")
                    ? game::PlayerType::AUTHENTICATED_USER : game::PlayerType::ADMIN;
            }
            
            if (player_type == game::PlayerType::INVALID) {
                event->push_result(packet->connection_descriptor, error::PACKET_RESULT_FAIL_LOGIN);
                continue;
            }

            const auto [player_id, success] = connection_env.register_player(player_search.get_player_identity_number());
            if (not success) {
                event->push_result(packet->connection_descriptor, error::PACKET_RESULT_ALREADY_LOGIN);
                continue;
            }

            packet->connection_descriptor->associate_game_player(
                player_id,
                player_type,
                packet->username,
                packet->password);

            event->push_result(packet->connection_descriptor, error::PACKET_RESULT_SUCCESS_LOGIN);
        }
    }
}