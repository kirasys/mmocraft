#include "pch.h"
#include "net/socket.h"

#include <map>
#include <cassert>

#include "logging/error.h"
#include "logging/logger.h"
#include "io/io_service.h"
#include "system_initializer.h"

namespace
{
    bool is_socket_system_initialized = false;
}

namespace net
{
    net::Socket::Socket() noexcept
        : _handle{ }
    { }

    net::Socket::Socket(SocketType type)
        : _handle{ create_windows_socket(type, WSA_FLAG_OVERLAPPED) }
    {
        if (not is_valid())
            throw error::SOCKET_CREATE;
    }

    net::Socket::Socket(win::Socket sock)
        : _handle{ sock }
    { }

    net::Socket::Socket(win::UniqueSocket&& sock)
        : _handle{ std::move(sock) }
    { }

    void net::Socket::initialize_system()
    {
        if (not is_socket_system_initialized) {
            WSADATA wsaData;
            int result = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
            CONSOLE_LOG_IF(fatal, result != 0) << "WSAStartup() failed: " << result;

            setup::add_termination_handler([]() {
                ::WSACleanup();
                });

            is_socket_system_initialized = true;
        }
    }

    bool net::Socket::bind(std::string_view ip, int port)
    {
        return true;
    }

    bool net::Socket::listen(int backlog)
    {
        return true;
    }

    error::ErrorCode net::Socket::accept(io::IoAcceptEvent& event)
    {
        return error::SUCCESS;
    }

    bool net::Socket::send(win::Socket sock, WSAOVERLAPPED* overlapped, WSABUF* wsa_buf, DWORD buffer_count)
    {
        return true;
    }

    bool net::Socket::send(WSAOVERLAPPED* overlapped, WSABUF* wsa_buf, DWORD buffer_count)
    {
        return true;
    }

    bool net::Socket::recv(win::Socket sock, WSAOVERLAPPED* overlapped, WSABUF* wsa_buf, DWORD buffer_count)
    {
        return true;
    }

    bool net::Socket::recv(WSAOVERLAPPED* overlapped, WSABUF* wsa_buf, DWORD buffer_count)
    {
        return true;
    }

    void net::Socket::close() noexcept
    {
        _handle.reset();
    }

    win::Socket net::create_windows_socket(SocketType type, DWORD flags)
    {
        switch (type) {
        case SocketType::TCPv4:
            return WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, flags);
            break;
        case SocketType::UDPv4:
            return ::WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, flags);
            break;
        default:
            return INVALID_SOCKET;
        }
    }
}