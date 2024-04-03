#pragma once

#include <iostream>
#include <sstream>
#include <utility>
#include <source_location>

namespace error
{
	struct ErrorCode
	{
		enum Network
		{
			SUCCESS = 0,			// success must be 0
			CREATE_SOCKET_ERROR,
			BIND_ERROR,
			LISTEN_ERROR,
			ACCEPTEX_LOAD_ERROR,
			ACCEPTEX_FAIL_ERROR,
		};
	};

	class RuntimeException
	{
	public:
		virtual std::string_view what() const = 0;
	protected:
		std::stringstream m_message{ "" };
	};

	class NetworkException : public RuntimeException
	{
	public:
		NetworkException(ErrorCode::Network code,
			int os_error_code = ::WSAGetLastError(),
			std::string_view summary = "",
			const std::source_location& location = std::source_location::current());

		virtual std::string_view what() const
		{
			return m_message.view();
		}
	};

	struct IoError : RuntimeException
	{

	};

}