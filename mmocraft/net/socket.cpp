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

    RIO_EXTENSION_FUNCTION_TABLE Rio_fn = { 0 };
}

const RIO_EXTENSION_FUNCTION_TABLE& net::rio_api()
{
    return Rio_fn;
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

void net::Socket::close() noexcept
{
    _handle.reset();
}

void net::Socket::reset(SocketProtocol protocol)
{
    _handle.reset(create_windows_socket(protocol));
}

void net::Socket::initialize_system()
{
    if (not is_socket_system_initialized) {
        WSADATA wsaData;
        int result = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
        CONSOLE_LOG_IF(fatal, result != 0) << "WSAStartup() failed: " << result;

        setup::add_termination_handler([]() {
            ::WSACleanup();
        });
        
        win::UniqueSocket sock{ create_windows_socket(SocketProtocol::TCPv4Rio) };

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

        {
            GUID guid = WSAID_MULTIPLE_RIO;
            DWORD bytes = 0;

            auto res = ::WSAIoctl(
                sock.get(),
                SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER,
                &guid,
                sizeof(guid),
                &Rio_fn,
                sizeof(Rio_fn),
                &bytes, NULL, NULL);

            CONSOLE_LOG_IF(fatal, res == SOCKET_ERROR) << "Fail to get pointer of rio function table: " << res;
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
        return false;

    return true;
}

bool net::Socket::listen(int backlog) {
    if (::listen(_handle, backlog) == SOCKET_ERROR)
        return false;

    return true;
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

    return success || ERROR_IO_PENDING == ::WSAGetLastError();
}

bool net::Socket::accept(win::Socket listen_sock, win::Socket accepted_sock, std::byte* buf, WSAOVERLAPPED* overlapped)
{
    DWORD bytes_received;
    BOOL success = AcceptEx_fn(
        listen_sock, accepted_sock,
        LPVOID(buf),
        0, // does not recevice packet data.
        sizeof(SOCKADDR_STORAGE) + 16, sizeof(SOCKADDR_STORAGE) + 16,
        &bytes_received,
        overlapped
    );

    return success || ERROR_IO_PENDING == ::WSAGetLastError();
}

bool net::Socket::send(win::Socket sock, std::byte* buf, std::size_t buf_size, WSAOVERLAPPED* overlapped)
{
    WSABUF wbuf[1];
    wbuf[0].buf = reinterpret_cast<char*>(buf);
    wbuf[0].len = ULONG(buf_size);

    DWORD flags = 0;

    int ret = ::WSASend(
        sock,
        wbuf, std::size(wbuf),
        NULL,
        flags,
        overlapped,
        NULL
    );

    return ret == 0 || ERROR_IO_PENDING == ::WSAGetLastError();
}

bool net::Socket::recv(win::Socket sock, std::byte* buf, std::size_t buf_size, WSAOVERLAPPED* overlapped)
{
    WSABUF wbuf[1];
    wbuf[0].buf = reinterpret_cast<char*>(buf);
    wbuf[0].len = ULONG(buf_size);

    DWORD flags = 0;

    int ret = ::WSARecv(
        sock,
        wbuf, std::size(wbuf),
        NULL,
        &flags,
        overlapped,
        NULL
    );

    return ret == 0 || ERROR_IO_PENDING == ::WSAGetLastError();
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

int net::Socket::recv_from(char* buf, std::size_t buf_size)
{
    sockaddr_in sock_addr;
    int sock_addr_size = sizeof(sock_addr);

    return ::recvfrom(
        _handle,
        buf,
        static_cast<int>(buf_size),
        0,
        reinterpret_cast<SOCKADDR*>(&sock_addr),
        &sock_addr_size
    );
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
    case SocketProtocol::TCPv4Rio:
        return ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_REGISTERED_IO | WSA_FLAG_OVERLAPPED);
    default:
        return INVALID_SOCKET;
    }
}