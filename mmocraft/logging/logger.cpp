#include "Logger.h"

#include <iostream>
#include <map>

namespace Logging {
	LogLevel string_to_level(std::string log_level) {
		static std::map<std::string, LogLevel> log_level_map = {
			{"DEBUG", LogLevel::Debug},
			{"INFO", LogLevel::Info},
			{"WARN", LogLevel::Warn},
			{"ERROR", LogLevel::Error},
			{"FATAL", LogLevel::Fatal},
		};
		
		if (log_level_map.find(log_level) == log_level_map.end())
			return LogLevel::Info; // Default

		return log_level_map[log_level];
	}

	Logger::Logger(const char* log_output_file = nullptr) {
		if (log_output_file) {
			_log_stream.open(log_output_file, std::ofstream::out);
			if (_log_stream.fail()) {
				std::cerr << "Error opening file: " << log_output_file;
				return;
			}
		}
	}

	Logger::~Logger() {
		if (_log_stream.is_open())
			_log_stream.close();

	}
}