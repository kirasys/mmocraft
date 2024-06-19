#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string_view>
#include <ctime>
#include <unordered_map>

#include "game/player.h"
#include "io/io_event_pool.h"
#include "io/io_service.h"
#include "net/socket.h"
#include "net/connection_key.h"
#include "win/smart_handle.h"
#include "util/common_util.h"
#include "util/interval_task.h"

namespace net
{
    class PacketHandleServer;

    class ConnectionEnvironment;

    enum SenderType
    {
        Tick_Thread,
        Any_Thread,
        SenderType_Count,
    };

    class Connection : public io::IoEventHandler, util::NonCopyable, util::NonMovable
    {
        // The minecrft beta server will disconnect a client,
        // if it doesn't receive at least one packet before 1200 in-game ticks (1200 tick = 60s)
        static constexpr unsigned REQUIRED_MILLISECONDS_FOR_EXPIRE = 60 * 1000;
        static constexpr unsigned REQUIRED_MILLISECONDS_FOR_SECURE_DELETION = 5 * 1000;

    public:
        struct Descriptor : util::NonCopyable, util::NonMovable
        {
            Descriptor() = default;

            ~Descriptor();

            Descriptor(net::Connection*,
                net::ConnectionKey, 
                net::ConnectionEnvironment&,
                win::UniqueSocket&&,
                io::IoService&,
                io::IoRecvEvent*,
                win::ObjectPool<io::IoSendEvent>::Pointer[]);

            inline bool is_online() const
            {
                return online;
            }

            ConnectionKey connection_key() const
            {
                return _connection_key;
            }

            game::Player* get_connected_player()
            {
                return _player;
            }

            void set_offline(std::size_t current_tick = util::current_monotonic_tick());

            bool is_expired(std::size_t current_tick = util::current_monotonic_tick()) const;

            bool is_safe_delete(std::size_t current_tick = util::current_monotonic_tick()) const;

            bool try_interact_with_client();

            void update_last_interaction_time(std::size_t current_tick = util::current_monotonic_tick())
            {
                last_interaction_tick = current_tick;
            }

            void emit_receive_event(io::IoRecvEvent*);

            void emit_send_event(io::IoSendEvent*);

            bool emit_multicast_send_event(io::IoSendEventSharedData*);

            void multicast_send(io::IoSendEventSharedData*);

            bool disconnect(SenderType, std::string_view);

            bool disconnect(SenderType, error::ResultCode);

            void on_handshake_success(game::Player*);

            bool send_packet(SenderType, const net::PacketHandshake&);

            bool send_packet(SenderType, const net::PacketLevelInit&);

            bool send_packet(SenderType, const net::PacketSetPlayerID&);

            static void flush_send(net::ConnectionEnvironment&);

            static void flush_receive(net::ConnectionEnvironment&);

        private:
            net::ConnectionKey _connection_key;
            net::ConnectionEnvironment& connection_env;

            net::Socket client_socket;

            io::IoRecvEvent* io_recv_event = {};
            io::IoSendEvent* io_send_events[SenderType_Count] = {};

            std::vector<io::IoSendEventSharedData*> multicast_datas;
            static constexpr unsigned num_of_multicast_event = 8;
            std::vector<io::IoSendEvent> io_multicast_send_events;

            bool online = false;
            std::size_t last_offline_tick = 0;
            std::size_t last_interaction_tick = 0;

            std::mutex multicast_data_append_lock;

            game::Player* _player;
        };

        Connection(PacketHandleServer&, ConnectionKey, ConnectionEnvironment&, win::UniqueSocket&&, io::IoService& , io::IoEventPool&);

        ~Connection() = default;

        bool is_valid() const
        {
            return send_event_lockfree_data && send_events[SenderType_Count - 1] && io_recv_event;
        }

        error::ResultCode get_last_error() const
        {
            return last_error_code;
        }

        std::size_t process_packets(std::byte*, std::byte*);

        /**
         *  Event Handler Interface
         */

        virtual void on_error() override;

        virtual void on_complete(io::IoRecvEvent*) override;

        virtual std::size_t handle_io_event(io::IoRecvEvent*) override;

        virtual void on_complete(io::IoSendEvent*) override;

        //virtual std::optional<std::size_t> handle_io_event(io::IoSendEvent*) override;

    private:
        error::ResultCode last_error_code;

        net::PacketHandleServer& packet_handle_server;

        win::ObjectPool<io::IoSendEventData>::Pointer send_event_data;
        win::ObjectPool<io::IoSendEventLockFreeData>::Pointer send_event_lockfree_data;
        win::ObjectPool<io::IoSendEvent>::Pointer send_events[SenderType_Count];

        win::ObjectPool<io::IoRecvEventData>::Pointer io_recv_event_data;
        win::ObjectPool<io::IoRecvEvent>::Pointer io_recv_event;

     public:
        Descriptor descriptor;
    };

    class PacketHandleServer
    {
    public:
        virtual error::ResultCode handle_packet(Connection::Descriptor&, net::Packet*) = 0;
    };

    class PacketHandleServerStub : public PacketHandleServer
    {
        error::ResultCode handle_packet(Connection::Descriptor&, net::Packet*) override
        {
            return error::SUCCESS;
        }
    };
}