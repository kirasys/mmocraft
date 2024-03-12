#include "logger.h"

#include <iostream>
#include <map>

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
				logging::cerr() << logging::ErrorMessage::FILE_OPEN_ERROR << log_output_file;
				return;
			}
		}
	}

	Logger::~Logger()
	{
		if (m_log_stream.is_open())
			m_log_stream.close();

	}
}