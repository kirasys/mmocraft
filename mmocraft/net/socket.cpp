#include "pch.h"
#include "socket.h"

#include <map>
#include <cassert>

#include "logging/error.h"
#include "io/io_context.h"
#include "io/io_service.h"

using namespace error;

net::Socket::Socket() noexcept
	: m_handle{ }
{ }

net::Socket::Socket(SocketType type)
	: m_handle { create_windows_socket(type, WSA_FLAG_OVERLAPPED) }
{
	if (not is_valid())
		throw NetworkException(ErrorCode::SOCKET_CREATE);
}

net::Socket::Socket(win::Socket sock)
	: m_handle{ sock }
{ }

net::Socket::Socket(win::UniqueSocket&& sock)
	: m_handle{ std::move(sock) }
{ }

bool net::Socket::bind(std::string_view ip, int port){
	sockaddr_in sock_addr;
	sock_addr.sin_family = get_address_family();
	sock_addr.sin_port = ::htons(port);
	::inet_pton(get_address_family(), ip.data(), &sock_addr.sin_addr);

	if (::bind(m_handle, reinterpret_cast<SOCKADDR*>(&sock_addr), sizeof(sock_addr)) == SOCKET_ERROR)
		throw NetworkException(ErrorCode::SOCKET_BIND);

	return true;
}

bool net::Socket::listen(int backlog) {
	if (::listen(m_handle, backlog) == SOCKET_ERROR)
		throw NetworkException(ErrorCode::SOCKET_LISTEN);

	return true;
}

bool net::Socket::accept(io::IoAcceptContext& ctx)
{
	if (ctx.fnAcceptEx == nullptr) {
		GUID acceptex_guid = WSAID_ACCEPTEX;
		DWORD bytes = 0;

		if (::WSAIoctl(
			m_handle,
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&acceptex_guid,
			sizeof(acceptex_guid),
			&ctx.fnAcceptEx,
			sizeof(ctx.fnAcceptEx),
			&bytes, NULL, NULL) == SOCKET_ERROR)
		{
			throw NetworkException(ErrorCode::SOCKET_ACCEPTEX_LOAD);
		}
	}

	ctx.accepted_socket = create_windows_socket(SocketType::TCPv4, WSA_FLAG_OVERLAPPED);
	if (ctx.accepted_socket == INVALID_SOCKET)
		throw NetworkException(ErrorCode::SOCKET_CREATE);

	DWORD bytes_received;
	BOOL success = ctx.fnAcceptEx(
		m_handle, ctx.accepted_socket,
		LPVOID(ctx.buffer),
		0, // does not recevice packet data.
		sizeof(SOCKADDR_STORAGE) + 16, sizeof(SOCKADDR_STORAGE) + 16,
		&bytes_received,
		&ctx.overlapped
	);

	if (not success && (ERROR_IO_PENDING != ::WSAGetLastError()))
		throw NetworkException(ErrorCode::SOCKET_ACCEPTEX);
	
	return true;
}

bool net::Socket::send(io::IoSendContext& ctx)
{
	WSABUF buffer;
	buffer.buf = reinterpret_cast<char*>(ctx.buffer);
	buffer.len = sizeof(ctx.buffer);

	DWORD flags = 0;

	int ret = ::WSASend(
		m_handle,
		&buffer, 1,
		NULL,
		flags,
		&ctx.overlapped,
		NULL
	);

	if (ret == SOCKET_ERROR && (ERROR_IO_PENDING != ::WSAGetLastError()))
		throw NetworkException(ErrorCode::SOCKET_SEND);

	return true;
}

bool net::Socket::recv(io::IoRecvContext& ctx)
{
	WSABUF buffer;
	buffer.buf = reinterpret_cast<char*>(ctx.buffer) + ctx.num_of_unconsumed_bytes;
	buffer.len = ULONG(sizeof(ctx.buffer) - ctx.num_of_unconsumed_bytes);

	DWORD flags = 0;

	int ret = ::WSARecv(
		m_handle,
		&buffer, 1,
		NULL,
		&flags,
		&ctx.overlapped,
		NULL
	);

	if (ret == SOCKET_ERROR && (ERROR_IO_PENDING != ::WSAGetLastError()))
		throw NetworkException(ErrorCode::SOCKET_RECV);

	return true;
}


void net::Socket::close() noexcept {
	m_handle.reset();
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