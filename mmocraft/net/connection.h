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
#include "net/connection_key.h"
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
            Descriptor() = default;

            ~Descriptor();

            Descriptor(net::Connection*,
                net::ConnectionKey, 
                net::ConnectionEnvironment&,
                win::UniqueSocket&&,
                io::IoService&,
                io::IoRecvEvent*,
                io::IoSendEvent*,
                io::IoSendEvent*);

            inline bool is_online() const
            {
                return online;
            }

            ConnectionKey connection_key() const
            {
                return _connection_key;
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

            bool disconnect_immediate(std::string_view);

            bool disconnect_deferred(std::string_view);

            bool send_handshake_packet(const net::PacketHandshake&, SendType send_type = SendType::DEFERRED) const;

            static void flush_server_message(net::ConnectionEnvironment&);

            static void flush_client_message(net::ConnectionEnvironment&);

        private:
            net::ConnectionKey _connection_key;
            net::ConnectionEnvironment& connection_env;

            net::Socket client_socket;

            io::IoRecvEvent* io_recv_event = {};
            io::IoSendEvent* io_send_events[2] = {};

            bool online = false;
            std::size_t last_offline_tick = 0;
            std::size_t last_interaction_tick = 0;
        };

        Connection(PacketHandleServer&, ConnectionKey, ConnectionEnvironment&, win::UniqueSocket&&, io::IoCompletionPort& , io::IoEventPool&);

        ~Connection() = default;

        bool is_valid() const
        {
            return io_send_event && io_recv_event;
        }

        error::ResultCode get_last_error() const
        {
            return last_error_code;
        }

        std::size_t process_packets(std::byte*, std::byte*);

        /**
         *  Event Handler Interface
         */

        virtual void on_complete(io::IoRecvEvent*) override;

        virtual std::size_t handle_io_event(io::IoRecvEvent*) override;

        virtual void on_complete(io::IoSendEvent*) override;

        //virtual std::optional<std::size_t> handle_io_event(io::IoSendEvent*) override;

    private:
        error::ResultCode last_error_code;

        net::PacketHandleServer& packet_handle_server;

        win::ObjectPool<io::IoSendEventData>::Pointer io_send_event_data;
        win::ObjectPool<io::IoSendEvent>::Pointer io_send_event;

        win::ObjectPool<io::IoSendEventSmallData>::Pointer io_immedidate_send_event_data;
        win::ObjectPool<io::IoSendEvent>::Pointer io_immedidate_send_event;

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