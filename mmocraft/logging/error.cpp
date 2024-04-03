#include "pch.h"
#include "error.h"

#include <map>
#include <filesystem>
#include <cstdlib>

namespace error
{
	std::string_view to_string(Exception::Network code)
	{
		static std::map< Exception::Network, const char*> error_code_map = {
			{ Exception::Network::SUCCESS, "SUCCESS"},
			{ Exception::Network::CREATE_SOCKET, "CREATE_SOCKET_ERROR"},
			{ Exception::Network::BIND, "BIND_ERROR"},
			{ Exception::Network::LISTEN, "LISTEN_ERROR"},
			{ Exception::Network::ACCEPTEX_LOAD, "ACCEPTEX_LOAD_ERROR"},
			{ Exception::Network::ACCEPTEX_FAIL, "ACCEPTEX_FAIL_ERROR"},
			{ Exception::Network::SEND, "SEND_ERROR"},
			{ Exception::Network::RECV, "RECV_ERROR"},
		};
		return error_code_map[code];
	}

	std::ostream& operator<<(std::ostream& os, Exception::Network ex)
	{
		return os << to_string(ex) << '(' << ::WSAGetLastError() << ')';
	}
}