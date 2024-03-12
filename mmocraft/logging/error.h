#pragma once

#include <iostream>
#include <sstream>
#include <utility>
#include <source_location>

namespace logging
{
	enum ErrorMessage {
		INVALID_SOCKET_ERROR
	};

	class ErrorStream
	{
	public:
		ErrorStream(const std::source_location&);

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
		std::stringstream m_buf;
	};

	ErrorStream cerr(const std::source_location& location = std::source_location::current());
}