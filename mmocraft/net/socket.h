#pragma once

#include <iostream>
#include <string_view>

#include "io/io_event.h"
#include "win/win_type.h"
#include "win/win_base_object.h"
#include "win/smart_handle.h"
#include "util/common_util.h"
#include "logging/error.h"

namespace net
{
    enum SocketProtocol
    {
        None,
        TCPv4,
        TCPv4Overlapped,
        UDPv4,
        UDPv4Overlapped,
    };

    class Socket : public win::WinBaseObject<win::Socket>, util::NonCopyable
    {
    public:
        // constructor
        Socket() noexcept;
        Socket(SocketProtocol);
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

        static void initialize_system();

        void close() noexcept;

        bool bind(std::string_view, int);

        bool listen(int backlog = SOMAXCONN);

        error::ErrorCode accept(io::IoAcceptEvent&);

        bool connect(std::string_view, int, WSAOVERLAPPED*);

        bool send(WSAOVERLAPPED*, WSABUF*);

        bool send_to(const char* ip, int port, const char* data, std::size_t data_size);

        bool recv(WSAOVERLAPPED*, WSABUF*);

        bool recv_from(const char* ip, int port, char* buf, std::size_t buf_size);

        int get_address_family() {
            return AF_INET; // TODO: IPv6
        }

    private:
        win::UniqueSocket _handle;
    };

    win::Socket create_windows_socket(SocketProtocol);
}