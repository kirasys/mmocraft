#include "pch.h"
#include "error.h"

namespace error
{
	static const char* to_string(ErrorCode code)
	{
		switch (code) {
		// Socket
		case ErrorCode::SOCKET_CREATE:		  return "SOCKET_CREATE";
		case ErrorCode::SOCKET_BIND:		  return "SOCKET_BIND";
		case ErrorCode::SOCKET_LISTEN:		  return "SOCKET_LISTEN";
		case ErrorCode::SOCKET_ACCEPTEX_LOAD: return "SOCKET_ACCEPTEX_LOAD";
		case ErrorCode::SOCKET_ACCEPTEX:	  return "SOCKET_ACCEPTEX";
		case ErrorCode::SOCKET_SEND:		  return "SOCKET_SEND";
		case ErrorCode::SOCKET_RECV:		  return "SOCKET_RECV";

		// IO Service
		case ErrorCode::IO_SERVICE_CREATE_COMPLETION_PORT: return "IO_SERVICE_CREATE_COMPLETION_PORT";

		// Connection
		case ErrorCode::CONNECTION_CREATE: "CONNECTION_CREATE";

		default: return nullptr;
		}
	}

	std::ostream& operator<<(std::ostream& os, ErrorCode code)
	{
		if (auto msg = to_string(code))
			return os << msg;
		return os << int(code);
	}
}