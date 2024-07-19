#pragma once

#include <unordered_set>

#include "io/io_event.h"
#include "io/io_service.h"
#include "net/socket.h"
#include "net/connection.h"

namespace bench
{

    enum ClientState
    {
        Error,

        Initialized,

        Connected,

        Handshake_Requested,
        Handshake_Completed,

        Level_Initialized,
    };

    class ClientBot final : public io::IoEventHandler
    {
    public:

        ClientState state() const
        {
            return _state;
        }

        void set_state(ClientState after)
        {
            _state = after;
        }

        ClientBot(io::IoService&);

        bool connect(std::string_view ip, int port);

        void disconnect();

        void send_handshake();

        void send_ping();

        void send_random_block(util::Coordinate3D pos);

        bool is_safe_delete() const
        {
            return last_interactin_at + 5 * 1000 < util::current_monotonic_tick();
        }

        /* IOEvent handlers */

        virtual void on_error() override;

        virtual std::size_t handle_io_event(io::IoAcceptEvent*) override;

        virtual void on_complete(io::IoAcceptEvent*) override;

        virtual std::size_t handle_io_event(io::IoSendEvent*) override;

        virtual void on_complete(io::IoSendEvent*, std::size_t) override;

        virtual std::size_t handle_io_event(io::IoRecvEvent*) override;

        virtual void on_complete(io::IoRecvEvent*) override;

    private:
        int _id;

        /* Client Status */
        ClientState _state = ClientState::Initialized;
        std::size_t total_received_bytes = 0;
        std::size_t total_sended_bytes = 0;
        std::size_t last_interactin_at = 0;
        
        std::size_t remain_packet_size = 0;

        std::unique_ptr<net::ConnectionIO> connection_io;

        io::IoAcceptEvent io_accept_event;
    };
}