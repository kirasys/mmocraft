#include "pch.h"
#include "master_server.h"

#include <array>
#include <cstring>

#include "config/config.h"
#include "proto/config.pb.h"
#include "logging/error.h"
#include "database/query.h"

namespace net
{
    MasterServer::MasterServer(const config::Configuration_Server& server_conf)
        : connection_env{ server_conf.max_player() }
        , io_service { io::DEFAULT_NUM_OF_CONCURRENT_EVENT_THREADS }
        , server_core{ *this, connection_env, io_service, server_conf }
        , database_core{ }
        , world{ connection_env }
        
        , deferred_handshake_packet_task{ &MasterServer::handle_deferred_handshake_packet, this }
    {

    }

    error::ResultCode MasterServer::handle_packet(net::Connection::Descriptor& conn_descriptor, Packet* packet)
    {
        switch (packet->id) {
        case PacketID::Handshake:
            return handle_handshake_packet(conn_descriptor, *static_cast<PacketHandshake*>(packet));
        default:
            CONSOLE_LOG(error) << "Unimplemented packet id: " << int(packet->id);
            return error::PACKET_UNIMPLEMENTED_ID;
        }
    }

    error::ResultCode MasterServer::handle_handshake_packet(net::Connection::Descriptor& conn_descriptor, PacketHandshake& packet)
    {
        deferred_handshake_packet_task.push_packet(conn_descriptor.connection_key(), packet);
        return error::PACKET_HANDLE_DEFERRED;
    }

    void MasterServer::tick()
    {
        Connection::Descriptor::flush_send(connection_env);
        Connection::Descriptor::flush_receive(connection_env);

        flush_deferred_packet();
    }

    void MasterServer::serve_forever()
    {
        const auto& db_conf = config::get_database_config();

        // start database system.
        if (not database_core.connect_with_password(db_conf))
            throw error::DATABASE_CONNECT;

        // start network I/O system.
        server_core.start_network_io_service();

        // load world map.
        const auto& world_conf = config::get_world_config();

        world.load_filesystem_world(world_conf.save_dir());

        while (1) {
            std::size_t start_tick = util::current_monotonic_tick();

            this->tick();
            world.tick(io_service);

            std::size_t end_tick = util::current_monotonic_tick();

            if (auto diff = end_tick - start_tick; diff < 1000)
                util::sleep_ms(std::max(1000 - diff, std::size_t(100)));
        }
    }

    void MasterServer::flush_deferred_packet()
    {
        for (auto task : deferred_packet_tasks) {
            if (task->exists() && task->transit_state(io::Task::Unused, io::Task::Processing)) {
                io_service.schedule_task(task);
            }
        }
    }

    /**
     *  Deferred packet handler methods.
     */

    void MasterServer::handle_deferred_handshake_packet(io::Task* task, const DeferredPacket<PacketHandshake>* packet_head)
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
            
            // return handshake result to clients.

            if (auto desc = connection_env.try_acquire_descriptor(packet->connection_key)) {
                if (player_type == game::PlayerType::INVALID) {
                    desc->disconnect(net::ThreadType::Any_Thread, error::PACKET_RESULT_FAIL_LOGIN);
                    continue;
                }

                auto player = world.add_player(
                        packet->connection_key,
                        player_search.get_player_identity(),
                        player_type,
                        packet->username,
                        packet->password);

                if (player == nullptr) {
                    desc->disconnect(net::ThreadType::Any_Thread, error::PACKET_RESULT_ALREADY_LOGIN);
                    continue;
                }

                desc->on_handshake_success(player);
            }
        }
    }
}