#pragma once
#pragma once
#include <fstream>

namespace logging {
	enum class LogLevel {
		Debug,
		Info,
		Warn,
		Error,
		Fatal,
	};

	LogLevel string_to_level(std::string log_level);

	class Logger final {
	public:
		Logger(const char* log_output_file);
		~Logger();
	private:
		std::ofstream m_log_stream;
	};
}