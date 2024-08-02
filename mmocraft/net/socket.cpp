#include "pch.h"
#include "socket.h"

#include <map>
#include <cassert>

#include "logging/error.h"
#include "logging/logger.h"
#include "io/io_service.h"
#include "system_initializer.h"

namespace
{
    bool is_socket_system_initialized = false;

    LPFN_ACCEPTEX AcceptEx_fn;

    LPFN_CONNECTEX ConnectEx_fn;
}

net::Socket::Socket() noexcept
    : _handle{ }
{ }

net::Socket::Socket(SocketProtocol protocol)
    : _handle{ create_windows_socket(protocol) }
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

        win::UniqueSocket sock{ create_windows_socket(SocketProtocol::TCPv4Overlapped) };

        {
            GUID guid = WSAID_ACCEPTEX;
            DWORD bytes = 0;

            auto res = ::WSAIoctl(
                sock.get(),
                SIO_GET_EXTENSION_FUNCTION_POINTER,
                &guid,
                sizeof(guid),
                &AcceptEx_fn,
                sizeof(AcceptEx_fn),
                &bytes, NULL, NULL);

            CONSOLE_LOG_IF(fatal, res == SOCKET_ERROR) << "Fail to get pointer of AcceptEx: " << res;
        }

        {
            GUID guid = WSAID_CONNECTEX;
            DWORD bytes = 0;

            auto res = ::WSAIoctl(
                sock.get(),
                SIO_GET_EXTENSION_FUNCTION_POINTER,
                &guid,
                sizeof(guid),
                &ConnectEx_fn,
                sizeof(ConnectEx_fn),
                &bytes, NULL, NULL);

            CONSOLE_LOG_IF(fatal, res == SOCKET_ERROR) << "Fail to get pointer of ConnectEx: " << res;
        }

        is_socket_system_initialized = true;
    }
}

bool net::Socket::bind(std::string_view ip, int port){
    sockaddr_in sock_addr;
    sock_addr.sin_family = get_address_family();
    sock_addr.sin_port = ::htons(port);
    ::inet_pton(get_address_family(), ip.data(), &sock_addr.sin_addr);

    if (::bind(_handle, reinterpret_cast<SOCKADDR*>(&sock_addr), sizeof(sock_addr)) == SOCKET_ERROR)
        throw error::SOCKET_BIND;

    return true;
}

bool net::Socket::listen(int backlog) {
    if (::listen(_handle, backlog) == SOCKET_ERROR)
        throw error::SOCKET_LISTEN;

    return true;
}

error::ErrorCode net::Socket::accept(io::IoAcceptEvent& event)
{
    event.accepted_socket = create_windows_socket(SocketProtocol::TCPv4Overlapped);
    if (event.accepted_socket == INVALID_SOCKET)
        return error::SOCKET_CREATE;

    DWORD bytes_received;
    BOOL success = AcceptEx_fn(
        _handle, event.accepted_socket,
        LPVOID(event.data->begin()),
        0, // does not recevice packet data.
        sizeof(SOCKADDR_STORAGE) + 16, sizeof(SOCKADDR_STORAGE) + 16,
        &bytes_received,
        &event.overlapped
    );

    return not success && (ERROR_IO_PENDING != ::WSAGetLastError()) ?
        error::SOCKET_ACCEPTEX : error::SUCCESS;
}

bool net::Socket::connect(std::string_view ip, int port, WSAOVERLAPPED* overlapped)
{
    sockaddr_in sock_addr;
    sock_addr.sin_family = get_address_family();
    sock_addr.sin_port = ::htons(port);
    ::inet_pton(get_address_family(), ip.data(), &sock_addr.sin_addr);

    BOOL success = ConnectEx_fn(
        _handle,
        reinterpret_cast<SOCKADDR*>(&sock_addr),
        sizeof(sock_addr),
        nullptr,
        0,
        nullptr,
        overlapped
    );

    return not success && (ERROR_IO_PENDING != ::WSAGetLastError()) ? false : true;
}

bool net::Socket::send(WSAOVERLAPPED* overlapped, WSABUF* wsa_buf)
{
    DWORD flags = 0;

    int ret = ::WSASend(
        _handle,
        wsa_buf, 1,
        NULL,
        flags,
        overlapped,
        NULL
    );

    if (ret == SOCKET_ERROR && (ERROR_IO_PENDING != ::WSAGetLastError()))
        return false;

    return true;
}

bool net::Socket::send_to(const char* ip, int port, const char* data, std::size_t data_size)
{
    sockaddr_in sock_addr;
    sock_addr.sin_family = get_address_family();
    sock_addr.sin_port = ::htons(port);
    ::inet_pton(get_address_family(), ip, &sock_addr.sin_addr);

    int ret = ::sendto(
        _handle, 
        data,
        static_cast<int>(data_size),
        0,
        reinterpret_cast<SOCKADDR*>(&sock_addr),
        sizeof(sock_addr)
    );

    return ret != SOCKET_ERROR;
}

bool net::Socket::recv(WSAOVERLAPPED* overlapped, WSABUF* wsa_buf)
{
    DWORD flags = 0;

    int ret = ::WSARecv(
        _handle,
        wsa_buf, 1,
        NULL,
        &flags,
        overlapped,
        NULL
    );

    if (ret == SOCKET_ERROR && (ERROR_IO_PENDING != ::WSAGetLastError()))
        return false;

    return true;
}

bool net::Socket::recv_from(const char* ip, int port, char* buf, std::size_t buf_size)
{
    sockaddr_in sock_addr;
    int sock_addr_size = sizeof(sock_addr);

    int ret = ::recvfrom(
        _handle,
        buf,
        static_cast<int>(buf_size),
        0,
        reinterpret_cast<SOCKADDR*>(&sock_addr),
        &sock_addr_size
    );

    return ret != SOCKET_ERROR;
}

void net::Socket::close() noexcept {
    _handle.reset();
}

bool net::Socket::set_socket_option(int optname, int optval)
{
    auto ret = ::setsockopt(_handle, SOL_SOCKET, optname, (char*)&optval, sizeof(optval));
    return ret != SOCKET_ERROR;
}

bool net::Socket::set_nonblocking_mode(bool mode)
{
    u_long iMode = mode ? 1 : 0;
    auto ret = ioctlsocket(_handle, FIONBIO, &iMode);
    return ret != SOCKET_ERROR;
}

win::Socket net::create_windows_socket(SocketProtocol protocol)
{
    switch (protocol) {
    case SocketProtocol::TCPv4:
        return ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    case SocketProtocol::TCPv4Overlapped:
        return ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    case SocketProtocol::UDPv4:
        return ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    case SocketProtocol::UDPv4Overlapped:
        return ::WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
    default:
        return INVALID_SOCKET;
    }
}