#pragma once
#include <string>
#include <tuple>

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
			int page_size = 0;
			int alllocation_granularity = 0;
			int num_of_processors = 0;
			MegaBytes memory_pool_size { 128 };
		} system;

		struct ServerConfig
		{
			int max_player = 100;
		} server;

		struct LogConfig
		{
			std::string level = "ERROR";
			std::string file_path = "server_log.txt";
		} log;

		struct LoginConfig
		{

		} login;

		struct DatabaseConfig
		{
			std::string driver_name;
			std::string server_address;
			std::string database;
			std::string userid;
			std::string password;
		} db;

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