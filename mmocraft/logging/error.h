#pragma once

#include <iostream>
#include <sstream>
#include <utility>
#include <source_location>

namespace logging
{
	enum ErrorMessage {
		FILE_OPEN_ERROR,
		INVALID_SOCKET_ERROR,
	};

	class ErrorStream
	{
	public:
		ErrorStream(const std::source_location&, bool exit_after_print = false);

		~ErrorStream();

		// template member function should be in the header.
		template <typename T>
		ErrorStream& operator<<(T&& value)
		{
			m_buf << std::forward<T>(value);
			return *this;
		}

		ErrorStream& operator<<(ErrorMessage);

		// no move/copy
		ErrorStream(ErrorStream&) = delete;
		ErrorStream(ErrorStream&&) = delete;
		ErrorStream& operator=(ErrorStream&) = delete;
		ErrorStream& operator=(ErrorStream&&) = delete;

	private:
		bool m_exit_after_print;
		std::stringstream m_buf;
	};

	ErrorStream cerr(const std::source_location& location = std::source_location::current());
	ErrorStream cfatal(const std::source_location& location = std::source_location::current());
}

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