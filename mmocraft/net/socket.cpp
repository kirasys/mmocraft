#include "socket.h"

#include <map>
#include <cassert>
#include <ws2tcpip.h>

#include "../logging/error.h"
#include "../io/io_context.h"
#include "../io/io_service.h"

using namespace error;

net::Socket::Socket() noexcept
	: m_type{ SocketType::None }, m_handle{ INVALID_SOCKET }
{ }

net::Socket::Socket(SocketType type)
	: m_type{ type }
{
	const DWORD flags = WSA_FLAG_OVERLAPPED;
	m_handle = create_windows_socket(type, flags);

	if (not is_valid())
		throw NetworkException(ErrorCode::Network::CREATE_SOCKET_ERROR);
}

net::Socket::~Socket()
{
	close();
}

net::Socket::Socket(net::Socket&& sock) noexcept {
	this->close();

	std::swap(m_handle, sock.m_handle);
	assert(not sock.is_valid());

	m_type = sock.m_type;
}

auto net::Socket::operator=(net::Socket&& sock) noexcept -> net::Socket& {
	this->close();
	
	std::swap(m_handle, sock.m_handle);
	assert(not sock.is_valid());

	m_type = sock.m_type;

	return *this;
}

auto net::Socket::bind(std::string_view ip, int port) -> ErrorCode::Network {
	sockaddr_in sock_addr;
	sock_addr.sin_family = get_address_family();
	sock_addr.sin_port = ::htons(port);
	::inet_pton(get_address_family(), ip.data(), &sock_addr.sin_addr);

	if (::bind(m_handle, reinterpret_cast<SOCKADDR*>(&sock_addr), sizeof(sock_addr)) == SOCKET_ERROR)
		throw NetworkException(ErrorCode::Network::BIND_ERROR);

	return ErrorCode::Network::SUCCESS;
}

auto net::Socket::listen(int backlog) -> ErrorCode::Network {
	if (::listen(m_handle, backlog) == SOCKET_ERROR)
		throw NetworkException(ErrorCode::Network::LISTEN_ERROR);

	return ErrorCode::Network::SUCCESS;
}

auto net::Socket::accept(io::AcceptIoContext &io_context) -> ErrorCode::Network
{
	DWORD bytes_received;

	if (io_context.fnAcceptEx == nullptr) {
		GUID acceptex_guid = WSAID_ACCEPTEX;
		DWORD bytes = 0;

		if (::WSAIoctl(
			m_handle,
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&acceptex_guid,
			sizeof(acceptex_guid),
			&io_context.fnAcceptEx,
			sizeof(io_context.fnAcceptEx),
			&bytes, NULL, NULL) == SOCKET_ERROR)
		{
			throw NetworkException(ErrorCode::Network::ACCEPTEX_LOAD_ERROR);
		}
	}

	if (not io_context.accepted_socket) {
		io_context.accepted_socket = create_windows_socket(m_type, WSA_FLAG_OVERLAPPED);
		if (io_context.accepted_socket == INVALID_SOCKET)
			throw NetworkException(ErrorCode::Network::CREATE_SOCKET_ERROR);
	}
	
	io_context.overlapped.Internal = 0;
	io_context.overlapped.InternalHigh = 0;
	io_context.overlapped.Offset = 0;
	io_context.overlapped.OffsetHigh = 0;
	io_context.overlapped.hEvent = NULL;

	//io_context.callback = 

	BOOL success = io_context.fnAcceptEx(
		m_handle, io_context.accepted_socket,
		LPVOID(io_context.buffer),
		sizeof(io_context.buffer) - (2 * (sizeof(SOCKADDR_STORAGE) + 16)),
		sizeof(SOCKADDR_STORAGE) + 16, sizeof(SOCKADDR_STORAGE) + 16,
		&bytes_received,
		&io_context.overlapped
	);

	if (not success && (ERROR_IO_PENDING != ::WSAGetLastError()))
		throw NetworkException(ErrorCode::Network::ACCEPTEX_FAIL_ERROR);
	
	return ErrorCode::Network::SUCCESS;
}

/*
auto net::Socket::accept_handler() -> Socket::ErrorCode
{

}
*/

void net::Socket::close() noexcept {
	if (is_valid()) {
		::closesocket(m_handle);
		m_handle = INVALID_SOCKET;
	}
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