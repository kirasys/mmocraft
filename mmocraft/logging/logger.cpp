#include "logger.h"

#include <iostream>
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

	Logger::Logger(const char* log_output_file = nullptr)
	{
		if (log_output_file) {
			m_log_stream.open(log_output_file, std::ofstream::out);
			if (m_log_stream.fail()) {
				logging::cerr() << "Fail to open file: " << log_output_file;
				return;
			}
		}
	}

	Logger::~Logger()
	{
		if (m_log_stream.is_open())
			m_log_stream.close();

	}

	LogStream::LogStream(std::ostream &os, const std::source_location& location, bool exit_after_print)
		: m_os(os), m_exit_after_print{ exit_after_print }
	{
		m_buf << std::filesystem::path(location.file_name()).filename() << '('
			<< location.line() << ':'
			<< location.column() << ") `"
			<< location.function_name() << "`: ";
	}

	LogStream::~LogStream()
	{
		// TODO: does it thread-safe?
		m_os << m_buf.view() << std::endl;

		if (m_exit_after_print)
			std::exit(0);
	}

	LogStream cerr(const std::source_location& location) {
		return LogStream{ std::cerr, location, false };
	}

	LogStream cfatal(const std::source_location& location) {
		return LogStream{ std::cerr, location, true };
	}
}