#include "mock_socket.h"
#include "net/socket.h"

#include "logging/error.h"
#include "logging/logger.h"
#include "io/io_service.h"
#include "system_initializer.h"

namespace
{
    bool is_socket_system_initialized = false;

    test::MockSocket* mock = nullptr;
}

test::MockSocket::MockSocket()
{
    mock = this;
}

test::MockSocket::~MockSocket()
{
    mock = nullptr;
}

net::Socket::Socket() noexcept
    : _handle{ }
{ }

net::Socket::Socket(SocketProtocol protocol)
    : _handle{ create_windows_socket(protocol, WSA_FLAG_OVERLAPPED) }
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

bool net::Socket::bind(std::string_view ip, int port) {
    return true;
}

bool net::Socket::listen(int backlog) {
    return true;
}

error::ErrorCode net::Socket::accept(io::IoAcceptEvent& event) {
    return error::SUCCESS;
}

bool net::Socket::connect(std::string_view ip, int port, WSAOVERLAPPED* overlapped)
{
    return error::SUCCESS;
}

bool net::Socket::send(WSAOVERLAPPED* overlapped, WSABUF* wsa_buf) {
    if (mock) mock->send(wsa_buf[0].buf, wsa_buf[0].len);
    return true;
}

bool net::Socket::recv(WSAOVERLAPPED* overlapped, WSABUF* wsa_buf) {
    if (mock) mock->recv(wsa_buf[0].buf, wsa_buf[0].len);
    return true;
}

void net::Socket::close() noexcept {
    _handle.reset();
}

win::Socket net::create_windows_socket(SocketProtocol protocol, DWORD flags)
{
    return ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, flags);
}