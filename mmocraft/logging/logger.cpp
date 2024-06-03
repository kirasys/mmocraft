#include "pch.h"
#include "logger.h"

#include <map>
#include <mutex>
#include <filesystem>
#include <string_view>

#include "error.h"

namespace
{
	logging::LogLevel system_log_level = logging::LogLevel::Info;
	std::ofstream system_log_file_stream;
	std::mutex system_log_mutex;
}

namespace logging
{
	LogLevel to_log_level(std::string log_level)
	{
		static const std::map<std::string, LogLevel> log_level_map = {
			{"DEBUG", LogLevel::Debug},
			{"INFO", LogLevel::Info},
			{"WARN", LogLevel::Warn},
			{"ERROR", LogLevel::Error},
			{"FATAL", LogLevel::Fatal},
		};
		
		if (log_level_map.find(log_level) == log_level_map.end())
			return LogLevel::Info; // Default

		return log_level_map.at(log_level);
	}

	void initialize_logging_system(std::string_view out_file_path, LogLevel level)
	{
		setlocale(LC_ALL, ""); // user-default ANSI code page obtained from the operating system

		system_log_level = level;
		system_log_file_stream.open(out_file_path, std::ofstream::out);
		if (not system_log_file_stream.is_open())
			logging::cfatal() << "Fail to open file: " << out_file_path;
	}

	/*  LogStream Class */

	LogStream::LogStream(std::ostream &os, const std::source_location &location, bool fatal_flag)
		: _os(os), _fatal_flag{ fatal_flag }
	{
		set_line_prefix(location);
	}

	LogStream::~LogStream()
	{
		{
			const std::lock_guard<std::mutex> lock(system_log_mutex);
			_os << _buf.view() << '\n';
		}

		if (_fatal_flag)
			std::exit(0);
	}

	void LogStream::set_line_prefix(const std::source_location& location)
	{
		_buf << std::filesystem::path(location.file_name()).filename() << '('
			<< location.line() << ':'
			<< location.column() << ") `"
			<< location.function_name() << "`: ";
	}

	LogStream cerr(const std::source_location &location) {
		return LogStream{ std::cerr, location, false };
	}

	LogStream cfatal(const std::source_location &location) {
		return LogStream{ std::cerr, location, true };
	}

	LogStream err(const std::source_location& location) {
		return LogStream{ system_log_file_stream, location, false };
	}

	LogStream fatal(const std::source_location& location) {
		return LogStream{ system_log_file_stream, location, true };
	}

	void logging_sql_error(SQLSMALLINT handle_type, SQLHANDLE handle, RETCODE error_code)
	{
		std::cout << "error_code: " << error_code << '\n';
		if (error_code == SQL_SUCCESS_WITH_INFO || error_code == SQL_ERROR) {
			SQLINTEGER native_error_code;
			WCHAR error_message[1024];
			WCHAR sql_state[SQL_SQLSTATE_SIZE + 1];

			for (SQLSMALLINT i = 1;
				::SQLGetDiagRec(handle_type,
					handle,
					i,
					sql_state,
					&native_error_code,
					error_message,
					(SQLSMALLINT)(sizeof(error_message) / sizeof(*error_message)),
					(SQLSMALLINT*)NULL) == SQL_SUCCESS;
				i++)
			{
				std::wcerr << error_message << L'\n';
				//fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, native_error);
			}
		}
	}
}