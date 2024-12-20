#pragma once

#include "game/world.h"

#include "net/connection.h"
#include "net/connection_environment.h"
#include "net/tcp_server.h"
#include "net/udp_server.h"
#include "net/packet_extension.h"

#include "database/couchbase_core.h"

#include "util/interval_task.h"

namespace net
{
    namespace game_server_task_interval {
        constexpr std::size_t chat_message        = 1 * 1000; // 1 seconds.
    }

    class GameServer : public net::PacketHandler, net::MessageHandler
    {
    public:
        static constexpr protocol::server_type_id server_type = protocol::server_type_id::game;

        using packet_handler_type = error::ResultCode (GameServer::*)(net::Connection&, util::byte_view packet_data);

        GameServer(unsigned max_clients, int num_of_event_threads = io::default_num_of_concurrent_event_threads);

        void tick();

        void announce_server();

        bool initialize(const char* router_ip, int router_port);

        void serve_forever(const char* router_ip, int router_port);

        void on_disconnect(net::Connection&) override;

        /* Packet handlers */

        error::ResultCode handle_packet(net::Connection&, const std::byte*) override;
        
        error::ResultCode handle_handshake_packet(net::Connection&, util::byte_view packet_data);

        error::ResultCode handle_ping_packet(net::Connection&, util::byte_view packet_data);

        error::ResultCode handle_two_way_ping_packet(net::Connection&, util::byte_view packet_data);

        error::ResultCode handle_ext_ping_packet(net::Connection&, util::byte_view packet_data);

        error::ResultCode handle_set_block_packet(net::Connection&, util::byte_view packet_data);

        error::ResultCode handle_player_position_packet(net::Connection&, util::byte_view packet_data);

        error::ResultCode handle_chat_message_packet(net::Connection&, util::byte_view packet_data);

        error::ResultCode handle_ext_info_packet(net::Connection&, util::byte_view packet_data);

        error::ResultCode handle_ext_entry_packet(net::Connection&, util::byte_view packet_data);

        /* Message handlers */

        virtual bool handle_message(net::MessageRequest&) override;

        io::DetachedTask handle_handshake_response_message(MessageRequest&);

    private:

        net::ConnectionEnvironment connection_env;

        io::RegisteredIO io_service;

        net::TcpServer tcp_server;
        net::UdpServer udp_server;

        game::World world;

        util::IntervalTaskScheduler<GameServer> interval_tasks;
    };
}