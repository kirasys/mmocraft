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
#include "io/multicast_manager.h"
#include "net/socket.h"
#include "net/packet_extension.h"
#include "net/connection_key.h"
#include "win/smart_handle.h"
#include "util/common_util.h"
#include "util/interval_task.h"

namespace net
{
    class PacketHandleServer;

    class ConnectionEnvironment;

    class Connection;

    struct ConnectionIO : util::NonCopyable, util::NonMovable
    {
        ConnectionIO() = default;

        ~ConnectionIO();

        ConnectionIO(net::ConnectionID, io::RegisteredIO&, win::UniqueSocket&&);

        bool is_receive_io_busy() const
        {
            return io_recv_event->is_processing;
        }

        bool is_send_io_busy() const
        {
            return io_send_event->is_processing;
        }

        void close();

        void register_event_handler(io::IoEventHandler*);

        bool post_connect_event(io::IoConnectEvent*, std::string_view ip, int port);

        bool post_recv_event();

        bool post_send_event();

        bool send_raw_data(const std::byte*, std::size_t) const;

        void send_multicast_data(io::MulticastDataEntry&);

        bool send_ping() const;

        void on_complete_event(io::IoMulticastSendEvent* event);

        template <typename PacketType>
        bool send_packet(const PacketType& packet) const
        {
            return packet.serialize(*io_send_event->event_data());
        }

        static void flush_send(net::ConnectionEnvironment&);

        void flush_multicast_send();

        static void flush_receive(net::ConnectionEnvironment&);

    private:
        io::RegisteredIO& io_service;

        net::ConnectionID connection_id;
        net::Socket client_socket;

        std::unique_ptr<io::IoRecvEvent> io_recv_event;
        std::unique_ptr<io::IoSendEvent> io_send_event;

        static constexpr std::size_t max_multicast_event_count = 32;
        std::mutex multicast_event_lock;
        std::vector<io::IoMulticastSendEvent*> ready_multicast_events;
        std::vector<io::IoMulticastSendEvent*> free_multicast_events;
    };

    class Connection : public io::IoEventHandler, util::NonCopyable, util::NonMovable
    {
        // The minecrft beta server will disconnect a client,
        // if it doesn't receive at least one packet before 1200 in-game ticks (1200 tick = 60s)
        static constexpr unsigned REQUIRED_MILLISECONDS_FOR_EXPIRE = 60 * 1000;
        static constexpr unsigned REQUIRED_MILLISECONDS_FOR_SECURE_DELETION = 5 * 1000;

    public:
        Connection(PacketHandleServer&, ConnectionKey, ConnectionEnvironment&, win::UniqueSocket&&, io::RegisteredIO&);

        ~Connection();

        bool is_valid() const
        {
            return connection_io.get() != nullptr;
        }

        error::ResultCode get_last_error() const
        {
            return last_error_code;
        }

        ConnectionKey connection_key() const
        {
            return _connection_key;
        }

        ConnectionID connection_id() const
        {
            return _connection_key.index();
        }

        ConnectionIO* io()
        {
            return connection_io.get(); 
        }

        game::Player* associated_player()
        {
            return _player.get();
        }

        void set_offline(std::size_t current_tick = util::current_monotonic_tick());

        inline bool is_online() const
        {
            return _is_online;
        }

        inline bool is_kicked() const
        {
            return _is_kicked;
        }

        bool is_expired(std::size_t current_tick = util::current_monotonic_tick()) const;

        bool is_safe_delete(std::size_t current_tick = util::current_monotonic_tick()) const;

        bool try_interact_with_client();

        void update_last_interaction_time(std::size_t current_tick = util::current_monotonic_tick())
        {
            last_interaction_tick = current_tick;
        }

        void disconnect();

        void kick(std::string_view);

        void kick(error::ResultCode);

        std::size_t process_packets(std::byte*, std::byte*);

        void set_player(std::unique_ptr<game::Player> player)
        {
            _player = std::move(player);
        }

        void on_handshake_success();

        /**
         *  Event Handler Interface
         */

        virtual void on_error() override;

        virtual void on_complete(io::IoRecvEvent*) override;

        virtual std::size_t handle_io_event(io::IoRecvEvent*) override;

        virtual void on_complete(io::IoSendEvent*, std::size_t) override;

        virtual void on_complete(io::IoMulticastSendEvent*, std::size_t) override;

    private:
        error::ResultCode last_error_code;

        net::PacketHandleServer& packet_handle_server;

        net::ConnectionKey _connection_key;
        net::ConnectionEnvironment& connection_env;
        std::unique_ptr<ConnectionIO> connection_io;
        
        bool _is_kicked = false;

        bool _is_online = false;

        std::size_t last_offline_tick = 0;
        std::size_t last_interaction_tick = 0;

        std::unique_ptr<game::Player> _player;
    };

    class PacketHandleServer
    {
    public:
        virtual error::ResultCode handle_packet(net::Connection&, const std::byte*) = 0;

        virtual void on_disconnect(net::Connection&) = 0;
    };

    class PacketHandleServerStub : public PacketHandleServer
    {
        error::ResultCode handle_packet(net::Connection&, const std::byte*) override
        {
            return error::SUCCESS;
        }

        void on_disconnect(net::Connection&) override
        {
            return;
        }
    };
}