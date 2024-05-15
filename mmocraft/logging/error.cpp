#include "pch.h"
#include "error.h"

#include <array>

namespace
{
	constinit const std::array<const char*, error::ErrorCode::SIZE> error_messages = [] {
		using namespace error;
		std::array<const char*, error::ErrorCode::SIZE> arr{};

		// Socket
		arr[ErrorCode::SOCKET_CREATE] = "SOCKET_CREATE";
		arr[ErrorCode::SOCKET_BIND] = "SOCKET_BIND";
		arr[ErrorCode::SOCKET_LISTEN] = "SOCKET_LISTEN";
		arr[ErrorCode::SOCKET_ACCEPTEX_LOAD] = "SOCKET_ACCEPTEX_LOAD";
		arr[ErrorCode::SOCKET_ACCEPTEX] = "SOCKET_ACCEPTEX";
		arr[ErrorCode::SOCKET_SEND] = "SOCKET_SEND";
		arr[ErrorCode::SOCKET_RECV] = "SOCKET_RECV";

		// Io Service
		arr[ErrorCode::IO_SERVICE_CREATE_COMPLETION_PORT] = "IO_SERVICE_CREATE_COMPLETION_PORT";

		// Connection
		arr[ErrorCode::CONNECTION_CREATE] = "CONNECTION_CREATE";

		return arr;
	}();
}

namespace error
{
	std::ostream& operator<<(std::ostream& os, ErrorCode code)
	{
		if (auto msg = error_messages[code])
			return os << msg;
		return os << int(code);
	}
}