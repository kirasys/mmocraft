#include "Logger.h"

#include <iostream>

namespace Logging {
	LogLevel string_to_level(std::string_view log_level) {
		if (log_level == "DEBUG")
			return LogLevel::kDebug;

		else if (log_level == "INFO")
			return LogLevel::kInfo;

		else if (log_level == "WARN")
			return LogLevel::kWarn;

		else if (log_level == "ERROR")
			return LogLevel::kError;

		else
			return LogLevel::kFatal;
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