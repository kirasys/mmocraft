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

	void initialize_system();

	class Logger : util::NonCopyable, util::NonMovable
	{
	public:
		Logger(bool is_fatal, std::ostream&, const std::source_location&);

		~Logger();
		
		template <typename T>
		void append(T&& value)
		{
			_buffer << std::forward<T>(value);
		}

		void flush();

	private:
		void set_line_prefix(const std::source_location&);

		bool _is_fatal;
		std::ostream& _output_stream;
		std::stringstream _buffer;
	};

	class LogStream
	{
	public:
		LogStream(bool is_fatal, std::ostream&, const std::source_location&);

		~LogStream();

		// template member function should be in the header.
		template <typename T>
		LogStream& operator<<(T&& value)
		{
			logger.append(std::forward<T>(value));
			return *this;
		}

	private:
		Logger logger;
	};

	class ConditionalLogStream
	{
	public:
		ConditionalLogStream(bool condition, bool is_fatal, std::ostream&, const std::source_location&);

		~ConditionalLogStream();

		// template member function should be in the header.
		template <typename T>
		ConditionalLogStream& operator<<(T&& value)
		{
			if (_condition) 
				logger.append(std::forward<T>(value));
			return *this;
		}

	private:
		bool _condition;
		Logger logger;
	};


	// Console log functions
	LogStream cerr(const std::source_location &location = std::source_location::current());
	
	LogStream cfatal(const std::source_location &location = std::source_location::current());

	ConditionalLogStream cerr(bool condition, const std::source_location& location = std::source_location::current());

	ConditionalLogStream cfatal_if(bool condition, const std::source_location& location = std::source_location::current());


	// File log functions
	LogStream err(const std::source_location& location = std::source_location::current());
	
	ConditionalLogStream err_if(bool condition, const std::source_location& location = std::source_location::current());

	LogStream fatal(const std::source_location& location = std::source_location::current());

	ConditionalLogStream fatal_if(bool condition, const std::source_location& location = std::source_location::current());

	void logging_sql_error(SQLSMALLINT handle_type, SQLHANDLE handle, RETCODE error_code);
}