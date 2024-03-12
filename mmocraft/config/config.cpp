#include "config.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <string_view>
#include <algorithm>

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
			if (line.size() && line[0] == '#')
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

	// public functions

	bool load_config(Configuration& conf) {
		auto [config_map, success] = read_config(config_file_path);
		if (!success) return false;

		conf.loaded = true;

		if (config_map.find("log_level") != config_map.end())
			conf.log.level = logging::string_to_level(config_map["log_level"]);
		if (config_map.find("log_file_path") != config_map.end())
			conf.log.file_path = config_map["log_file_path"];

		return true;
	}

	const Configuration& get_config() {
		static Configuration configuration;

		if (!configuration.loaded) {
			if (bool fail = !load_config(configuration))
				std::cerr << "Fail to load configuration file at \"" << config_file_path << "\"" << std::endl;
		}
		return configuration;
	}
}