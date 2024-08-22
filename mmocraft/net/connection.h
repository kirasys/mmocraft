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
#include "net/packet_extension.h"
#include "net/connection_key.h"
#include "win/smart_handle.h"
#include "util/common_util.h"
#include "util/interval_task.h"

namespace net
{
    class PacketHandleServer;

    class ConnectionEnvironment;

    struct ConnectionIO : util::NonCopyable, util::NonMovable
    {
        ConnectionIO() = default;

        ~ConnectionIO();

        ConnectionIO(win::UniqueSocket&&);

        void register_event_handler(io::IoService&, io::IoEventHandler* event_handler);

        bool is_receive_io_busy() const
        {
            return io_recv_event.is_processing;
        }

        bool is_send_io_busy() const
        {
            return io_send_event.is_processing;
        }

        void close();

        bool emit_connect_event(io::IoAcceptEvent*, std::string_view ip, int port);

        void emit_receive_event();

        void emit_receive_event(io::IoRecvEvent* event);

        void emit_send_event();

        void emit_send_event(io::IoSendEvent*);

        bool emit_multicast_send_event(io::IoSendEventSharedData*);

        bool send_raw_data(const std::byte*, std::size_t) const;

        bool send_disconnect_message(std::string_view);

        bool send_ping() const;

        template <typename PacketType>
        bool send_packet(const PacketType& packet) const
        {
            return packet.serialize(*io_send_event.data);
        }

        static void flush_send(net::ConnectionEnvironment&);

        static void flush_receive(net::ConnectionEnvironment&);

    private:
        net::Socket client_socket;

        io::IoRecvEventData io_recv_data;
        io::IoSendEventLockFreeData io_send_data;

        io::IoRecvEvent io_recv_event{ &io_recv_data };
        io::IoSendEvent io_send_event{ &io_send_data };

        static constexpr unsigned num_of_multicast_event = 8;
        std::vector<io::IoSendEvent> io_multicast_send_events;

        // it is used to prevent data interleaving problem when multiple threads sending together.
        // Todo: performance benchmark versus lockfree stack version.
        std::mutex send_event_lock;
    };

    class Connection : public io::IoEventHandler, util::NonCopyable, util::NonMovable
    {
        // The minecrft beta server will disconnect a client,
        // if it doesn't receive at least one packet before 1200 in-game ticks (1200 tick = 60s)
        static constexpr unsigned REQUIRED_MILLISECONDS_FOR_EXPIRE = 60 * 1000;
        static constexpr unsigned REQUIRED_MILLISECONDS_FOR_SECURE_DELETION = 5 * 1000;

    public:
        Connection(PacketHandleServer&, ConnectionKey, ConnectionEnvironment&, win::UniqueSocket&&, io::IoService&);

        ~Connection();

        bool is_valid() const
        {
            return connection_io.get() != nullptr;
        }

        inline bool is_online() const
        {
            return online;
        }

        error::ResultCode get_last_error() const
        {
            return last_error_code;
        }

        ConnectionKey connection_key() const
        {
            return _connection_key;
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

        bool is_expired(std::size_t current_tick = util::current_monotonic_tick()) const;

        bool is_safe_delete(std::size_t current_tick = util::current_monotonic_tick()) const;

        bool try_interact_with_client();

        void update_last_interaction_time(std::size_t current_tick = util::current_monotonic_tick())
        {
            last_interaction_tick = current_tick;
        }

        void disconnect();

        void disconnect_with_message(std::string_view);

        void disconnect_with_message(error::ResultCode);

        std::size_t process_packets(std::byte*, std::byte*);

        void set_player(std::unique_ptr<game::Player> player)
        {
            _player = std::move(player);
        }

        void on_handshake_success();

        void send_supported_cpe_list();

        /**
         *  Event Handler Interface
         */

        virtual void on_error() override;

        virtual void on_complete(io::IoRecvEvent*) override;

        virtual std::size_t handle_io_event(io::IoRecvEvent*) override;

        virtual void on_complete(io::IoSendEvent*, std::size_t) override;

    private:
        error::ResultCode last_error_code;

        net::PacketHandleServer& packet_handle_server;

        net::ConnectionKey _connection_key;
        net::ConnectionEnvironment& connection_env;
        std::unique_ptr<ConnectionIO> connection_io;

        bool online = false;
        std::size_t last_offline_tick = 0;
        std::size_t last_interaction_tick = 0;

        std::unique_ptr<game::Player> _player;
    };

    class PacketHandleServer
    {
    public:
        virtual error::ResultCode handle_packet(net::Connection&, const std::byte*) = 0;
    };

    class PacketHandleServerStub : public PacketHandleServer
    {
        error::ResultCode handle_packet(net::Connection&, const std::byte*) override
        {
            return error::SUCCESS;
        }
    };
}