#include "pch.h"
#include "error.h"

#include <map>
#include <filesystem>
#include <cstdlib>

namespace error
{
	static std::string_view to_string(Exception::Network code)
	{
		static std::map< Exception::Network, const char*> error_code_map = {
			{ Exception::Network::SUCCESS, "SUCCESS"},

			// Socket
			{ Exception::Network::SOCKET_CREATE, "SOCKET_CREATE"},
			{ Exception::Network::SOCKET_BIND, "SOCKET_BIND"},
			{ Exception::Network::SOCKET_LISTEN, "SOCKET_LISTEN"},
			{ Exception::Network::SOCKET_ACCEPTEX_LOAD, "SOCKET_ACCEPTEX_LOAD"},
			{ Exception::Network::SOCKET_ACCEPTEX, "SOCKET_ACCEPTEX"},
			{ Exception::Network::SOCKET_SEND, "SOCKET_SEND"},
			{ Exception::Network::SOCKET_RECV, "SOCKET_RECV"},

		};
		return error_code_map[code];
	}

	std::ostream& operator<<(std::ostream& os, Exception::Network ex)
	{
		return os << to_string(ex) << '(' << ::WSAGetLastError() << ')';
	}
}