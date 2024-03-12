#pragma once
#include <string>
#include <tuple>

#include "../logging/logger.h"

namespace config {
	struct Configuration {
		bool loaded;

		struct LogConfig {
			logging::LogLevel level = logging::LogLevel::Info;
			std::string file_path = "log\\";
		} log;

		struct LoginConfig {

		} login;

		Configuration() :loaded(false)
		{

		}

		// only one global configuration variable exists.
		Configuration(Configuration&) = delete;
		Configuration(Configuration&&) = delete;
		Configuration& operator=(Configuration&) = delete;
		Configuration& operator=(Configuration&&) = delete;
	};

	bool load_config(Configuration&);

	const Configuration& get_config();
}