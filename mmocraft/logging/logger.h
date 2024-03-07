#pragma once
#pragma once
#include <fstream>

namespace Logging {
	enum class LogLevel {
		kDebug,
		kInfo,
		kWarn,
		kError,
		kFatal,
	};

	LogLevel string_to_level(std::string_view log_level);

	class Logger final {
	public:
		Logger(const char* log_output_file);
		~Logger();
	private:
		std::ofstream _log_stream;
	};
}