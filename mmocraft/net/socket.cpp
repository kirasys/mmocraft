#include "pch.h"
#include "socket.h"

#include <map>
#include <cassert>

#include "logging/error.h"
#include "io/io_context.h"
#include "io/io_service.h"

using namespace error;

net::Socket::Socket() noexcept
	: m_type{ SocketType::None }, m_handle{ }
{ }

net::Socket::Socket(SocketType type)
	: m_type { type }
	, m_handle { create_windows_socket(type, WSA_FLAG_OVERLAPPED) }
{
	if (not is_valid())
		throw NetworkException(ErrorCode::Network::CREATE_SOCKET_ERROR);
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

auto net::Socket::accept(io::IoContext &io_ctx) -> ErrorCode::Network
{
	auto& accept_ctx = io_ctx.details.accept;

	if (accept_ctx.fnAcceptEx == nullptr) {
		GUID acceptex_guid = WSAID_ACCEPTEX;
		DWORD bytes = 0;

		if (::WSAIoctl(
			m_handle,
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&acceptex_guid,
			sizeof(acceptex_guid),
			&accept_ctx.fnAcceptEx,
			sizeof(accept_ctx.fnAcceptEx),
			&bytes, NULL, NULL) == SOCKET_ERROR)
		{
			throw NetworkException(ErrorCode::Network::ACCEPTEX_LOAD_ERROR);
		}
	}

	accept_ctx.accepted_socket = create_windows_socket(m_type, WSA_FLAG_OVERLAPPED);
	if (accept_ctx.accepted_socket == INVALID_SOCKET)
		throw NetworkException(ErrorCode::Network::CREATE_SOCKET_ERROR);

	DWORD bytes_received;
	BOOL success = accept_ctx.fnAcceptEx(
		m_handle, accept_ctx.accepted_socket,
		LPVOID(io_ctx.buffer),
		sizeof(io_ctx.buffer) - (2 * (sizeof(SOCKADDR_STORAGE) + 16)),
		sizeof(SOCKADDR_STORAGE) + 16, sizeof(SOCKADDR_STORAGE) + 16,
		&bytes_received,
		&io_ctx.overlapped
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