#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <source_location>

#include <sql.h>
#include <sqlext.h>

#include "util/common_util.h"

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

	void initialize_logging_system(std::string_view out_file_path, LogLevel level);

	class LogStream : util::NonCopyable, util::NonMovable
	{
	public:
		LogStream(std::ostream &os, const std::source_location&, bool m_fatal_flag = false);

		~LogStream();

		// template member function should be in the header.
		template <typename T>
		LogStream& operator<<(T&& value)
		{
			_buf << std::forward<T>(value);
			return *this;
		}

	private:
		std::ostream& _os;

		bool _fatal_flag;
		std::stringstream _buf;
	};

	// Console log functions
	LogStream cerr(const std::source_location &location = std::source_location::current());
	LogStream cfatal(const std::source_location &location = std::source_location::current());

	void logging_sql_error(SQLSMALLINT handle_type, SQLHANDLE handle, RETCODE error_code);
}