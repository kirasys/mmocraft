#include "socket.h"

#include <ws2tcpip.h>

net::Socket::Socket() noexcept
	: m_af(0), m_handle(INVALID_SOCKET)
{ }

net::Socket::Socket(int af, int type, int protocol)
	: m_af(af), m_handle(WSASocketW(af, type, protocol, NULL, 0, WSA_FLAG_OVERLAPPED))
{ }

net::Socket::~Socket()
{
	close();
}

net::Socket::Socket(net::Socket&& sock) noexcept {
	this->close();
	m_af = sock.m_af;
	std::swap(m_handle, sock.m_handle);
}

auto net::Socket::operator=(net::Socket&& sock) noexcept -> net::Socket& {
	this->close();
	m_af = sock.m_af;
	std::swap(m_handle, sock.m_handle);
	return *this;
}

auto net::Socket::create_socket(net::SocketType type) -> net::Socket {
	switch (type) {
	case TCPv4:
		return net::Socket{ AF_INET, SOCK_STREAM, IPPROTO_TCP };
	case UDPv4:
		return net::Socket{ AF_INET, SOCK_DGRAM, IPPROTO_UDP };
	default:
		return net::Socket{};
	}
}

auto net::Socket::bind(std::string_view ip, int port) -> Socket::ErrorCode {
	sockaddr_in sock_addr;
	sock_addr.sin_family = m_af;
	sock_addr.sin_port = ::htons(port);
	::inet_pton(m_af, ip.data(), &sock_addr.sin_addr);

	if (::bind(m_handle, reinterpret_cast<SOCKADDR*>(&sock_addr), sizeof(sock_addr)) == SOCKET_ERROR)
		return ::WSAGetLastError();
	return Socket::OpSuccess;
}

auto net::Socket::listen(int backlog) -> Socket::ErrorCode {
	if (::listen(m_handle, backlog) == SOCKET_ERROR)
		return ::WSAGetLastError();
	return Socket::OpSuccess;
}

void net::Socket::close() noexcept {
	if (is_valid()) {
		::closesocket(m_handle);
		m_handle = INVALID_SOCKET;
	}
}