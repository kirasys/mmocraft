#pragma once

#include "database/database_core.h"

#include "game/world.h"

#include "net/connection.h"
#include "net/connection_environment.h"
#include "net/deferred_packet.h"
#include "net/tcp_server.h"
#include "net/udp_server.h"
#include "net/packet_extension.h"

namespace net
{
    static constexpr std::size_t user_authentication_task_interval = 3 * 1000; // 3 seconds.
    static constexpr std::size_t chat_message_task_interval = 1 * 1000; // 1 seconds.

    class MasterServer : public net::PacketHandleServer, public net::MessageHandler
    {
    public:
        using handler_type = error::ResultCode (MasterServer::*)(net::Connection&, const std::byte*, std::size_t);

        MasterServer(net::ConnectionEnvironment&, io::IoCompletionPort&);

        void tick();

        void serve_forever(const char* router_ip, int router_port);

        /* Packet handlers */

        error::ResultCode handle_packet(net::Connection&, const std::byte*) override;
        
        error::ResultCode handle_handshake_packet(net::Connection&, const std::byte*, std::size_t);

        error::ResultCode handle_ping_packet(net::Connection&, const std::byte*, std::size_t);

        error::ResultCode handle_set_block_packet(net::Connection&, const std::byte*, std::size_t);

        error::ResultCode handle_player_position_packet(net::Connection&, const std::byte*, std::size_t);

        error::ResultCode handle_chat_message_packet(net::Connection&, const std::byte*, std::size_t);

        error::ResultCode handle_ext_info_packet(net::Connection&, const std::byte*, std::size_t);

        error::ResultCode handle_ext_entry_packet(net::Connection&, const std::byte*, std::size_t);

        /* Message handlers */

        //bool handle_message(const MessageRequest&, MessageResponse&) override;

        /**
         *  Deferred packet handler methods.
         */

        void handle_deferred_handshake_packet(io::Task*, const DeferredPacket<net::PacketHandshake>*);

        void handle_deferred_chat_message_packet(io::Task*, const DeferredPacket<net::PacketChatMessage>*);

    private:

        void flush_deferred_packet();

        net::ConnectionEnvironment& connection_env;

        io::IoCompletionPort& io_service;

        net::TcpServer tcp_server;
        net::UdpServer<MasterServer> udp_server;

        database::DatabaseCore database_core;

        game::World world;

        DeferredPacketTask<net::PacketHandshake, MasterServer> deferred_handshake_packet_task;
        DeferredPacketTask<net::PacketChatMessage, MasterServer> deferred_chat_message_packet_task;

        io::Task *deferred_packet_tasks[2] = {
            &deferred_handshake_packet_task,
            &deferred_chat_message_packet_task
        };
    };
}