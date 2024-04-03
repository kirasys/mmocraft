#include "pch.h"
#include "error.h"

#include <map>
#include <filesystem>
#include <cstdlib>

namespace error
{
	static std::string_view to_string(ErrorCode code)
	{
		static std::map<decltype(code), const char*> error_code_map = {
			// Socket
			{ ErrorCode::SOCKET_CREATE, "SOCKET_CREATE"},
			{ ErrorCode::SOCKET_BIND, "SOCKET_BIND"},
			{ ErrorCode::SOCKET_LISTEN, "SOCKET_LISTEN"},
			{ ErrorCode::SOCKET_ACCEPTEX_LOAD, "SOCKET_ACCEPTEX_LOAD"},
			{ ErrorCode::SOCKET_ACCEPTEX, "SOCKET_ACCEPTEX"},
			{ ErrorCode::SOCKET_SEND, "SOCKET_SEND"},
			{ ErrorCode::SOCKET_RECV, "SOCKET_RECV"},

			// IO Service
			{ ErrorCode::IO_SERVICE_CREATE_COMPLETION_PORT, "IO_SERVICE_CREATE_COMPLETION_PORT"},
		};
		return error_code_map[code];
	}

	std::ostream& operator<<(std::ostream& os, ErrorCode code)
	{
		return os << to_string(code);
	}
}