#include "pch.h"
#include "config.h"

#include <iostream>
#include <string>
#include <filesystem>
#include <google/protobuf/util/json_util.h>

#include "logging/error.h"
#include "logging/logger.h"
#include "util/protobuf_util.h"

namespace fs = std::filesystem;

namespace {
    config::GameConfig g_config;
}

namespace config {

    void set_default_configuration(config::Configuration_System& conf)
    {
        SYSTEM_INFO sys_info;
        ::GetSystemInfo(&sys_info);

        conf.set_page_size(sys_info.dwPageSize);
        conf.set_alllocation_granularity(sys_info.dwAllocationGranularity);
        conf.set_num_of_processors(sys_info.dwNumberOfProcessors);
    }

    void generate_config(const fs::path& config_path)
    {
        util::proto_message_to_json_file(get_config(), config_path);
    }

    void load_config(const fs::path& config_path)
    {
        if (not fs::exists(config_path)) {
            generate_config(config_path);
            std::cerr << "Configuration file is generated at \"" << config_path << "\". "
                << "Please fill in appropriate values.";
            std::exit(0);
        }

        util::json_file_to_proto_message(&get_config(), config_path);
    }

    config::GameConfig& get_config()
    {
        return g_config;
    }

    void initialize_system(std::string_view config_dir, std::string_view config_filename)
    {
        if (not fs::exists(config_dir))
            fs::create_directories(config_dir);

        load_config(fs::path(config_dir) / config_filename);
    }
}