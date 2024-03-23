#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <source_location>

#include "../util/common_util.h"

namespace logging
{
	enum class LogLevel
	{
		Debug,
		Info,
		Warn,
		Error,
		Fatal,
	};

	LogLevel to_log_level(std::string log_level);

	class Logger final
	{
		public:
			Logger(const char* log_output_file);
			~Logger();
		private:
			std::ofstream m_log_stream;
	};


	enum ErrorMessage {
		FILE_OPEN_ERROR,
		INVALID_SOCKET_ERROR,
	};


	class LogStream : util::NonCopyable, util::NonMovable
	{
	public:
		LogStream(std::ostream& os, const std::source_location&, bool exit_after_print = false);

		~LogStream();

		// template member function should be in the header.
		template <typename T>
		LogStream& operator<<(T&& value)
		{
			m_buf << std::forward<T>(value);
			return *this;
		}

		LogStream& operator<<(ErrorMessage);

	private:
		std::ostream& m_os;

		bool m_exit_after_print;
		std::stringstream m_buf;
	};

	LogStream cerr(const std::source_location& location = std::source_location::current());
	LogStream cfatal(const std::source_location& location = std::source_location::current());
}