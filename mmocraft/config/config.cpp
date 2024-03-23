#include "config.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <string_view>
#include <algorithm>
#define NOMINMAX
#include <Windows.h>

#include "logging/error.h"

namespace {
	void trim(std::string_view line) noexcept {
		line.remove_prefix(std::min(line.find_first_not_of(' '), line.size()));
		if (line.back() == ' ')
			line.remove_suffix(line.size() - line.find_last_not_of(' ') - 1);
	}
}

namespace config {
	using ConfigMap = std::map<std::string, std::string>;

	// global variables

	const char* config_file_path = "config/configuration.txt";

	// private functions

	ConfigMap parse_config(std::stringstream& config_ss) {
		ConfigMap config_map;

		std::string line;
		while (std::getline(config_ss >> std::ws, line)) {
			if (line.size() && (line[0] == '#' || line[0] == '['))
				continue;

			auto colon_pos = line.find(':');
			if (colon_pos == std::string::npos)
				continue;

			std::string_view key = line;
			key.remove_suffix(key.size() - colon_pos);
			trim(key);

			std::string_view value = line;
			value.remove_prefix(colon_pos + 1);
			trim(value);

			config_map[std::string(key)] = std::string(value);
		}

		return config_map;
	}

	std::tuple<ConfigMap, bool> read_config(const char* config_filepath) {
		std::ifstream config_stm(config_filepath, std::ifstream::in);

		if (config_stm.fail())
			return { {}, false };

		std::stringstream config_ss;
		config_ss << config_stm.rdbuf();

		return { parse_config(config_ss), true };
	}

	void load_log_config(Configuration::LogConfig& conf, ConfigMap conf_map)
	{
		auto end = conf_map.end();

		// Log configuration
		if (conf_map.find("log_level") != end)
			conf.level = logging::to_log_level(conf_map.at("log_level"));

		if (conf_map.find("log_file_path") != end)
			conf.file_path = conf_map.at("log_file_path");
	}

	void load_system_config(Configuration::SystemConfig conf, ConfigMap conf_map)
	{
		auto end = conf_map.end();

		{
			SYSTEM_INFO sys_info;
			GetSystemInfo(&sys_info);

			conf.page_size				 = sys_info.dwPageSize;
			conf.alllocation_granularity = sys_info.dwAllocationGranularity;
			conf.num_of_processors = sys_info.dwNumberOfProcessors;
		}

		if (conf_map.find("memory_pool_size") != end)
			conf.memory_pool_size = MegaBytes{ std::stoi(conf_map.at("memory_pool_size")) };
	}

	// public functions

	bool load_config(Configuration& conf) {
		auto [conf_map, success] = read_config(config_file_path);
		if (!success) return false;

		try {
			load_log_config(conf.log, conf_map);
			load_system_config(conf.system, conf_map);
		}
		catch (const std::exception& ex) {
			logging::cerr() << "Fail to setting config: " << ex.what();
			return false;
		}

		return true;
	}

	const Configuration& get_config() {
		static Configuration configuration;

		if (!configuration.loaded) {
			if (configuration.loaded = !load_config(configuration))
				logging::cfatal() << "Fail to load configuration file at \"" << config_file_path << "\"";
		}

		return configuration;
	}
}