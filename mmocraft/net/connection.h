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

namespace net
{
    class PacketHandleServer;
    class ConnectionEnvironment;

    class Connection : public io::IoEventHandler, util::NonCopyable, util::NonMovable
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
            friend Connection;
            friend ConnectionEnvironment;

            void set_offline(std::size_t current_tick = util::current_monotonic_tick());

            inline bool is_online() const
            {
                return online;
            }

            bool is_expired(std::size_t current_tick = util::current_monotonic_tick()) const;

            bool is_safe_delete(std::size_t current_tick = util::current_monotonic_tick()) const;

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

            void associate_game_player(game::PlayerID, game::PlayerType, const char* username, const char* password);

        private:
            net::Connection* connection;
            win::Socket raw_socket;

            io::IoRecvEvent* io_recv_event;
            io::IoSendEvent* io_send_events[2];

            std::unique_ptr<game::Player> self_player;

            bool online = false;
            std::size_t last_offline_tick = 0;
            std::size_t last_interaction_tick = 0;
        };

        Connection(PacketHandleServer&, ConnectionEnvironment&, win::UniqueSocket&&, io::IoCompletionPort& , io::IoEventPool&);

        ~Connection();

        bool is_valid() const
        {
            return io_send_event && io_recv_event;
        }

        std::size_t process_packets(std::byte*, std::byte*);

        void register_descriptor();

        /**
         *  Event Handler Interface
         */

        virtual void on_complete(io::IoRecvEvent*) override;

        virtual std::size_t handle_io_event(io::IoRecvEvent*) override;

        virtual void on_complete(io::IoSendEvent*) override;

        //virtual std::optional<std::size_t> handle_io_event(io::IoSendEvent*) override;

        Descriptor descriptor;

    private:
        net::PacketHandleServer& packet_handle_server;
        
        net::ConnectionEnvironment& connection_env;

        error::ResultCode last_error_code;

        net::Socket _client_socket;

        win::ObjectPool<io::IoSendEventData>::Pointer io_send_event_data;
        win::ObjectPool<io::IoSendEvent>::Pointer io_send_event;

        win::ObjectPool<io::IoSendEventSmallData>::Pointer io_immedidate_send_event_data;
        win::ObjectPool<io::IoSendEvent>::Pointer io_immedidate_send_event;

        win::ObjectPool<io::IoRecvEventData>::Pointer io_recv_event_data;
        win::ObjectPool<io::IoRecvEvent>::Pointer io_recv_event;
    };


    class PacketHandleServer
    {
    public:
        virtual error::ResultCode handle_packet(net::Connection::Descriptor&, net::Packet*) = 0;
    };

    class PacketHandleServerStub : public PacketHandleServer
    {
        error::ResultCode handle_packet(net::Connection::Descriptor&, net::Packet*) override
        {
            return error::SUCCESS;
        }
    };
}