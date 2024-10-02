#pragma once

#include <iostream>
#include <string_view>

#include "win/win_type.h"
#include "win/win_base_object.h"
#include "win/smart_handle.h"
#include "util/common_util.h"
#include "logging/error.h"

namespace io
{
    struct IoAcceptEvent;
}

namespace net
{
    enum class socket_protocol_id
    {
        none,
        tcp_v4,
        tcp_overlapped_v4,
        udp_v4,
        udp_overlapped_v4,
        tcp_rio_v4,
    };

    class Socket : public win::WinBaseObject<win::Socket>, util::NonCopyable
    {
    public:
        // constructor
        Socket() noexcept;
        Socket(net::socket_protocol_id);
        Socket(win::Socket);
        Socket(win::UniqueSocket&&);

        // destructor
        ~Socket() = default;

        // move controllers
        Socket(Socket&& sock) = default;
        Socket& operator=(Socket&&) = default;

        win::Socket get_handle() const {
            return _handle.get();
        }

        bool is_valid() const {
            return _handle.get();
        }

        static int get_address_family() {
            return AF_INET; // TODO: IPv6
        }

        static void initialize_system();

        void close() noexcept;

        void reset(net::socket_protocol_id);

        bool bind(std::string_view, int);

        bool listen(int backlog = SOMAXCONN);

        static bool connect(win::Socket, std::string_view, int, WSAOVERLAPPED*);

        static bool accept(win::Socket listen_sock, win::Socket accepted_sock, std::byte* buf, WSAOVERLAPPED*);

        static bool send(win::Socket, std::byte* buf, std::size_t buf_size, WSAOVERLAPPED*);

        static bool recv(win::Socket, std::byte* buf, std::size_t buf_size, WSAOVERLAPPED*);

        bool send_to(const char* ip, int port, const char* data, std::size_t data_size);

        int recv_from(char* buf, std::size_t buf_size);

        bool set_socket_option(int optname, int optval);

        bool set_nonblocking_mode(bool mode);

    private:
        win::UniqueSocket _handle;
    };

    class RioSocket
    {
    public:

    private:
        Socket _sock;

        RIO_RQ request_queue;
    };

    win::Socket create_windows_socket(net::socket_protocol_id);

    const RIO_EXTENSION_FUNCTION_TABLE& rio_api();
}