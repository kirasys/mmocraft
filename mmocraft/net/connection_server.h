#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string_view>
#include <ctime>
#include <unordered_map>

#include "game/player.h"
#include "io/io_event_pool.h"
#include "io/io_service.h"
#include "net/socket.h"
#include "win/smart_handle.h"
#include "util/common_util.h"
#include "util/interval_task.h"
#include "util/lockfree_stack.h"

namespace net
{
    class PacketHandleServer;

    class ConnectionServer : public io::IoEventHandler, util::NonCopyable, util::NonMovable
    {
        // The minecrft beta server will disconnect a client,
        // if it doesn't receive at least one packet before 1200 in-game ticks (1200 tick = 60s)
        static constexpr unsigned REQUIRED_MILLISECONDS_FOR_EXPIRE = 60 * 1000;
        static constexpr unsigned REQUIRED_MILLISECONDS_FOR_SECURE_DELETION = 5 * 1000;

    public:
        enum SendType
        {
            IMMEDIATE,
            DEFERRED,
        };

        struct Descriptor : util::NonCopyable, util::NonMovable
        {
            net::ConnectionServer* connection;
            win::Socket raw_socket;

            io::IoRecvEvent* io_recv_event;
            io::IoSendEvent* io_send_events[2];

            std::unique_ptr<game::Player> self_player;

            bool is_online = false;
            bool is_recv_event_running = false;
            std::size_t last_offline_tick = 0;
            std::size_t last_interaction_tick = 0;

            void set_offline();

            bool try_interact_with_client();

            void update_last_interaction_time(std::size_t current_tick = util::current_monotonic_tick())
            {
                last_interaction_tick = current_tick;
            }

            void activate_receive_cycle(io::IoRecvEvent*);

            void activate_send_cycle(io::IoSendEvent*);

            bool disconnect_immediate(std::string_view);

            bool disconnect_deferred(std::string_view);

            bool finalize_handshake() const;

            bool associate_game_player(game::PlayerID, game::PlayerType, const char* username, const char* password);
        };

        ConnectionServer(PacketHandleServer&, win::UniqueSocket&&, io::IoCompletionPort& , io::IoEventPool&);

        ~ConnectionServer();

        static void initialize_system();

        bool is_valid() const
        {
            return io_send_event && io_recv_event;
        }

        bool is_expired(std::size_t current_tick = util::current_monotonic_tick()) const;

        bool is_safe_delete(std::size_t current_tick = util::current_monotonic_tick()) const;

        void set_offline();

        std::size_t process_packets(std::byte*, std::byte*);

        void register_descriptor();

        static void tick();

        static void activate_pending_connections();

        static void cleanup_deleted_player();

        static void flush_server_message();

        static void flush_client_message();

        /**
         *  Event Handler Interface
         */

        virtual void on_complete(io::IoRecvEvent*) override;

        virtual std::size_t handle_io_event(io::IoRecvEvent*) override;

        virtual void on_complete(io::IoSendEvent*) override;

        //virtual std::optional<std::size_t> handle_io_event(io::IoSendEvent*) override;

    private:
        static util::IntervalTaskScheduler<void> connection_interval_tasks;
        static util::LockfreeStack<ConnectionServer::Descriptor*> accepted_connections;
        static std::unordered_map<Descriptor*, bool> online_connection_table;
        static std::unordered_map<game::PlayerID, game::Player*> player_lookup_table;

        net::PacketHandleServer& packet_handle_server;

        error::ResultCode last_error_code;

        net::Socket _client_socket;

        win::ObjectPool<io::IoSendEventData>::Pointer io_send_event_data;
        win::ObjectPool<io::IoSendEvent>::Pointer io_send_event;

        win::ObjectPool<io::IoSendEventSmallData>::Pointer io_immedidate_send_event_data;
        win::ObjectPool<io::IoSendEvent>::Pointer io_immedidate_send_event;

        win::ObjectPool<io::IoRecvEventData>::Pointer io_recv_event_data;
        win::ObjectPool<io::IoRecvEvent>::Pointer io_recv_event;

        Descriptor connection_descriptor;
    };


    class PacketHandleServer
    {
    public:
        virtual error::ResultCode handle_packet(net::ConnectionServer::Descriptor&, net::Packet*) = 0;
    };

    class PacketHandleServerStub : public PacketHandleServer
    {
        error::ResultCode handle_packet(net::ConnectionServer::Descriptor&, net::Packet*) override
        {
            return error::SUCCESS;
        }
    };
}