#pragma once

#include <iostream>
#include <string_view>

#include "io/io_event.h"
#include "win/win_type.h"
#include "win/win_base_object.h"
#include "win/smart_handle.h"
#include "util/common_util.h"
#include "logging/error.h"

namespace net
{
	enum SocketType
	{
		None,
		TCPv4,
		UDPv4,
	};

	class Socket : public win::WinBaseObject<win::Socket>, util::NonCopyable
	{
	public:
		// constructor
		Socket() noexcept;
		Socket(SocketType);
		Socket(win::Socket);
		Socket(win::UniqueSocket&&);

		// destructor
		~Socket() = default;

		// move controllers
		Socket(Socket&& sock) = default;
		Socket& operator=(Socket&&) = default;

		win::Socket get_handle() const {
			return _handle.get();
		}

		bool is_valid() const {
			return _handle.get();
		}

		static void initialize_system();

		void close() noexcept;

		bool bind(std::string_view, int);

		bool listen(int backlog = SOMAXCONN);

		bool accept(io::IoAcceptEvent&);

		static bool send(win::Socket, WSAOVERLAPPED*, WSABUF*, DWORD buffer_count);

		bool send(WSAOVERLAPPED*, WSABUF*, DWORD buffer_count);

		static bool recv(win::Socket, WSAOVERLAPPED*, WSABUF*, DWORD buffer_count);

		bool recv(WSAOVERLAPPED*, WSABUF*, DWORD buffer_count);

		int get_address_family() {
			return AF_INET; // TODO: IPv6
		}

	private:
		win::UniqueSocket _handle;
	};

	win::Socket create_windows_socket(SocketType, DWORD flags);
}