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
		arr[ErrorCode::SOCKET_SETOPT] = "SOCKET_SETOPT";

		// Io Service
		arr[ErrorCode::IO_SERVICE_CREATE_COMPLETION_PORT] = "IO_SERVICE_CREATE_COMPLETION_PORT";

		// Connection
		arr[ErrorCode::CLIENT_CONNECTION_CREATE] = "CLIENT_CONNECTION_CREATE";
		arr[ErrorCode::CLIENT_CONNECTION_FULL]   = "CLIENT_CONNECTION_FULL";

		// Database
		arr[ErrorCode::DATABASE_ALLOC_ENVIRONMENT_HANDLE] = "DATABASE_ALLOC_ENVIRONMENT_HANDLE";
		arr[ErrorCode::DATABASE_ALLOC_CONNECTION_HANDLE]  = "DATABASE_ALLOC_CONNECTION_HANDLE";
		arr[ErrorCode::DATABASE_ALLOC_STATEMENT_HANDLE]   = "DATABASE_ALLOC_STATEMENT_HANDLE";
		arr[ErrorCode::DATABASE_SET_ATTRIBUTE_VERSION]	  = "DATABASE_SET_ATTRIBUTE_VERSION";
		arr[ErrorCode::DATABASE_CONNECT] = "DATABASE_CONNECT";

		// Packet parsing
		arr[ErrorCode::PACKET_INVALID_ID]       = "Unsupported Packet ID";
		arr[ErrorCode::PACKET_UNIMPLEMENTED_ID] = "Unimplemented Packet ID";
		arr[ErrorCode::PACKET_INSUFFIENT_DATA]  = "PACKET_INSUFFIENT_DATA";

		// Packet validation
		arr[ErrorCode::PACKET_HANSHAKE_INVALID_PROTOCOL_VERSION] = "Unsupported protocol version";
		arr[ErrorCode::PACKET_HANSHAKE_IMPROPER_USERNAME_LENGTH] = "Username must be 1 to 16 characters";
		arr[ErrorCode::PACKET_HANSHAKE_IMPROPER_USERNAME_FORMAT] = "Username must be alphanumeric characters";
		arr[ErrorCode::PACKET_HANSHAKE_IMPROPER_PASSWORD_LENGTH] = "Password must be 1 to 32 characters";

		// Packet handling
		arr[ErrorCode::PACKET_HANDLE_ERROR]    = "PACKET_HANDLE_ERROR";
		arr[ErrorCode::PACKET_HANDLE_SUCCESS]  = "PACKET_HANDLE_SUCCESS";
		arr[ErrorCode::PACKET_HANDLE_DEFERRED] = "PACKET_HANDLE_DEFERRED";

		return arr;
	}();
}

namespace error
{
	const char* get_error_message(error::ErrorCode code)
	{
		return error_messages[code];
	}

	std::ostream& operator<<(std::ostream& os, ErrorCode code)
	{
		if (auto msg = error_messages[code])
			return os << msg;
		return os << int(code);
	}
}