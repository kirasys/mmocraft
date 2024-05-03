#include "pch.h"
#include "socket.h"

#include <map>
#include <cassert>

#include "logging/error.h"
#include "io/io_service.h"

using namespace error;

net::Socket::Socket() noexcept
	: _handle{ }
{ }

net::Socket::Socket(SocketType type)
	: _handle{ create_windows_socket(type, WSA_FLAG_OVERLAPPED) }
{
	if (not is_valid())
		throw NetworkException(ErrorCode::SOCKET_CREATE);
}

net::Socket::Socket(win::Socket sock)
	: _handle{ sock }
{ }

net::Socket::Socket(win::UniqueSocket&& sock)
	: _handle{ std::move(sock) }
{ }

bool net::Socket::bind(std::string_view ip, int port){
	sockaddr_in sock_addr;
	sock_addr.sin_family = get_address_family();
	sock_addr.sin_port = ::htons(port);
	::inet_pton(get_address_family(), ip.data(), &sock_addr.sin_addr);

	if (::bind(_handle, reinterpret_cast<SOCKADDR*>(&sock_addr), sizeof(sock_addr)) == SOCKET_ERROR)
		throw NetworkException(ErrorCode::SOCKET_BIND);

	return true;
}

bool net::Socket::listen(int backlog) {
	if (::listen(_handle, backlog) == SOCKET_ERROR)
		throw NetworkException(ErrorCode::SOCKET_LISTEN);

	return true;
}

bool net::Socket::accept(io::IoAcceptEvent& event)
{
	if (event.fnAcceptEx == nullptr) {
		GUID acceptex_guid = WSAID_ACCEPTEX;
		DWORD bytes = 0;

		if (::WSAIoctl(
			_handle,
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&acceptex_guid,
			sizeof(acceptex_guid),
			&event.fnAcceptEx,
			sizeof(event.fnAcceptEx),
			&bytes, NULL, NULL) == SOCKET_ERROR)
		{
			throw NetworkException(ErrorCode::SOCKET_ACCEPTEX_LOAD);
		}
	}

	event.accepted_socket = create_windows_socket(SocketType::TCPv4, WSA_FLAG_OVERLAPPED);
	if (event.accepted_socket == INVALID_SOCKET)
		throw NetworkException(ErrorCode::SOCKET_CREATE);

	DWORD bytes_received;
	BOOL success = event.fnAcceptEx(
		_handle, event.accepted_socket,
		LPVOID(event.data.begin()),
		0, // does not recevice packet data.
		sizeof(SOCKADDR_STORAGE) + 16, sizeof(SOCKADDR_STORAGE) + 16,
		&bytes_received,
		&event.overlapped
	);

	if (not success && (ERROR_IO_PENDING != ::WSAGetLastError()))
		throw NetworkException(ErrorCode::SOCKET_ACCEPTEX);
	
	return true;
}

bool net::Socket::send(io::IoSendEvent& event)
{
	WSABUF buffer;
	buffer.buf = reinterpret_cast<char*>(event.data.begin_unused());
	buffer.len = sizeof(event.data.unused_size());
	assert(buffer.len > 0);

	DWORD flags = 0;

	int ret = ::WSASend(
		_handle,
		&buffer, 1,
		NULL,
		flags,
		&event.overlapped,
		NULL
	);

	if (ret == SOCKET_ERROR && (ERROR_IO_PENDING != ::WSAGetLastError()))
		throw NetworkException(ErrorCode::SOCKET_SEND);

	return true;
}

bool net::Socket::recv(io::IoRecvEvent& event)
{
	WSABUF buffer;
	buffer.buf = reinterpret_cast<char*>(event.data.begin_unused());
	buffer.len = ULONG(event.data.unused_size());
	assert(buffer.len > 0);

	DWORD flags = 0;

	int ret = ::WSARecv(
		_handle,
		&buffer, 1,
		NULL,
		&flags,
		&event.overlapped,
		NULL
	);

	if (ret == SOCKET_ERROR && (ERROR_IO_PENDING != ::WSAGetLastError()))
		throw NetworkException(ErrorCode::SOCKET_RECV);

	return true;
}


void net::Socket::close() noexcept {
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