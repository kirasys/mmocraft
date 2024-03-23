#pragma once
#include <string>
#include <tuple>

#include "logging/logger.h"

namespace config {
	struct MegaBytes
	{
		int mb = 0;
		constexpr MegaBytes(int Mb) : mb(Mb) { }
		constexpr int to_bytes() const { return mb * 1048576; }
	};

	struct Configuration {
		bool loaded;

		struct SystemConfig
		{
			int page_size;
			int alllocation_granularity;
			int num_of_processors;
			MegaBytes memory_pool_size { 128 };
		} system;

		struct LogConfig
		{
			logging::LogLevel level = logging::LogLevel::Info;
			std::string file_path = "server_log.txt";
		} log;

		struct LoginConfig
		{

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