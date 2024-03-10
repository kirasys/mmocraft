#pragma once

#include <iostream>
#include <string_view>
#include <tuple>

#define NOMINMAX
#include <winsock2.h>

namespace net
{
	enum SocketType
	{
		TCPv4,
		UDPv4
	};

	class Socket
	{
	public:
		using ErrorCode = int;
		const ErrorCode OpSuccess = 0;
		
		// constructor
		Socket() noexcept;
		Socket(int af, int type, int protocol);

		// destructor
		~Socket();

		// copy controllers (deleted)
		Socket(Socket& dpc) = delete;
		Socket& operator=(Socket&) = delete;

		// move controllers
		Socket(Socket&& sock) noexcept;
		Socket& operator=(Socket&&) noexcept;

		static Socket create_socket(SocketType);

		inline bool is_valid() const {
			return m_handle != INVALID_SOCKET;
		}

		ErrorCode bind(std::string_view, int);

		ErrorCode listen(int backlog = SOMAXCONN);

		void close() noexcept;
	private:
		int m_af;
		SOCKET m_handle;
	};
}