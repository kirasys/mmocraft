#pragma once

#include <iostream>
#include <string_view>

#include "../io/io_context.h"
#include "../win/win_type.h"
#include "../win/win_base_object.h"

namespace net
{
	enum SocketType
	{
		None,
		TCPv4,
		UDPv4
	};

	enum SocketErrorCode
	{
		SUCCESS = 0, // success must be 0
		CREATE_SOCKET_ERROR,
		BIND_ERROR,
		LISTEN_ERROR,
		ACCEPTEX_LOAD_ERROR,
		ACCEPTEX_FAIL_ERROR,
	};

	std::ostream& operator<<(std::ostream&, SocketErrorCode);

	class Socket : public win::WinBaseObject<win::Socket>
	{
	public:
		// constructor
		Socket() noexcept;
		Socket(SocketType);

		// destructor
		~Socket();

		// copy controllers (deleted)
		Socket(Socket& dpc) = delete;
		Socket& operator=(Socket&) = delete;

		// move controllers
		Socket(Socket&& sock) noexcept;
		Socket& operator=(Socket&&) noexcept;

		win::Socket get_handle() const {
			return m_handle;
		}

		bool is_valid() const {
			return m_handle != INVALID_SOCKET;
		}

		void close() noexcept;

		SocketErrorCode bind(std::string_view, int);

		SocketErrorCode listen(int backlog = SOMAXCONN);

		SocketErrorCode accept(io::AcceptIoContext&);

		int get_address_family() {
			return AF_INET; // TODO: IPv6
		}

	private:
		SocketType m_type;
		win::Socket m_handle;
	};

	win::Socket create_windows_socket(SocketType, DWORD flags);
}