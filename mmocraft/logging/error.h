#pragma once

#include <iostream>
#include <sstream>
#include <utility>
#include <source_location>

namespace error
{
	enum ErrorCode
	{
		// Socket
		SOCKET_CREATE,
		SOCKET_BIND,
		SOCKET_LISTEN,
		SOCKET_ACCEPTEX_LOAD,
		SOCKET_ACCEPTEX,
		SOCKET_SEND,
		SOCKET_RECV,

		// IO Service
		IO_SERVICE_CREATE_COMPLETION_PORT,

		// Connection
		CONNECTION_CREATE,
	};

	struct Exception
	{
		const ErrorCode code;
	};

	struct NetworkException : Exception
	{
		NetworkException(ErrorCode code) noexcept
			: Exception{ code }
		{ }
	};

	struct IoException : Exception
	{
		IoException(ErrorCode code) noexcept
			: Exception{ code }
		{ }
	};

	std::ostream& operator<<(std::ostream& os, ErrorCode ex);
}