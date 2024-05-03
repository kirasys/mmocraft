#include "pch.h"
#include "logger.h"

#include <map>
#include <filesystem>

#include "error.h"

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

	struct Logger {
		LogLevel level = LogLevel::Debug;
		std::ofstream os;
	} g_logger;

	void init_logger(const char* out_file_path, LogLevel level)
	{
		g_logger.level = level;
		g_logger.os.open(out_file_path, std::ofstream::out);
		if (not g_logger.os.is_open())
			logging::cfatal() << "Fail to open file: " << out_file_path;
	}

	/*  LogStream Class */

	LogStream::LogStream(std::ostream &os, const std::source_location &location, bool fatal_flag)
		: _os(os), _fatal_flag{ fatal_flag }
	{
		_buf << std::filesystem::path(location.file_name()).filename() << '('
			<< location.line() << ':'
			<< location.column() << ") `"
			<< location.function_name() << "`: ";
	}

	LogStream::~LogStream()
	{
		// TODO: make it more thread-safe
		_os << _buf.view() << std::endl;

		if (_fatal_flag)
			std::exit(0);
	}

	LogStream cerr(const std::source_location &location) {
		return LogStream{ std::cerr, location, false };
	}

	LogStream cfatal(const std::source_location &location) {
		return LogStream{ std::cerr, location, true };
	}
}