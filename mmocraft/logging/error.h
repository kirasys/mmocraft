#pragma once

#include <iostream>
#include <sstream>
#include <utility>
#include <source_location>

namespace error
{
	enum ErrorCode
	{
		SUCCESS,

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

		// Database
		DATABASE_ALLOC_ENVIRONMENT_HANDLE,
		DATABASE_ALLOC_CONNECTION_HANDLE,
		DATABASE_ALLOC_STATEMENT_HANDLE,
		DATABASE_SET_ATTRIBUTE_VERSION,
		DATABASE_CONNECT,

		// Packet parsing
		PACKET_INVALID_ID,
		PACKET_INSUFFIENT_DATA,

		PACKET_HANSHAKE_INVALID_PROTOCOL_VERSION,
		PACKET_HANSHAKE_IMPROPER_USERNAME_LENGTH,
		PACKET_HANSHAKE_IMPROPER_USERNAME_FORMAT,
		PACKET_HANSHAKE_IMPROPER_PASSWORD_LENGTH,

		// Indicate size of the enum class.
		SIZE,
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

	const char* get_error_message(error::ErrorCode);

	std::ostream& operator<<(std::ostream& os, ErrorCode ex);
}