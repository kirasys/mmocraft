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
    config::Configuration g_configuration;
}

namespace config {
    void set_default_configuration()
    {
        set_default_configuration(get_server_config());
        set_default_configuration(get_world_config());
        set_default_configuration(get_log_config());
        set_default_configuration(get_database_config());
        set_default_configuration(get_system_config());
    }

    void set_default_configuration(config::Configuration_Server& conf)
    {
        conf.set_ip("127.0.0.1");
        conf.set_port(12345);
        conf.set_max_client(1000);
        conf.set_server_name("Massive Minecraft Classic Server");
        conf.set_motd("welcome to mmocraft server.");
    }

    void set_default_configuration(config::Configuration_World& conf)
    {
        conf.set_width(256);
        conf.set_height(256);
        conf.set_length(256);
        conf.set_save_dir("world\\");
    }

    void set_default_configuration(config::Configuration_Log& conf)
    {
        conf.set_log_filename("server.log");
        conf.set_log_dir("log");
    }

    void set_default_configuration(config::Configuration_Database& conf)
    {
        conf.set_driver_name("ODBC Driver 17 for SQL Server");
        conf.set_database_name("mmocraft");
        conf.set_userid("mmocraft_login");
        conf.set_password("12341234");
    }

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
        set_default_configuration();
        util::proto_message_to_json_file(g_configuration, config_path);
    }

    void load_config(const fs::path& config_path)
    {
        if (not fs::exists(config_path)) {
            generate_config(config_path);
            std::cerr << "Configuration file is generated at \"" << config_path << "\". "
                << "Please fill in appropriate values.";
            std::exit(0);
        }

        util::json_file_to_proto_message(&g_configuration, config_path);
    }

    Configuration& get_config()
    {
        return g_configuration;
    }

    Configuration_Server& get_server_config()
    {
        return *g_configuration.mutable_server();
    }

    Configuration_World& get_world_config()
    {
        return *g_configuration.mutable_world();
    }

    Configuration_Database& get_database_config()
    {
        return *g_configuration.mutable_database();
    }

    Configuration_Log& get_log_config()
    {
        return *g_configuration.mutable_log();
    }

    Configuration_System& get_system_config()
    {
        auto& system_conf = *g_configuration.mutable_system();
        if (system_conf.page_size() == 0)
            set_default_configuration(system_conf);

        return system_conf;
    }

    void initialize_system(std::string_view config_dir, std::string_view config_filename)
    {
        if (not fs::exists(config_dir))
            fs::create_directories(config_dir);

        load_config(fs::path(config_dir) / config_filename);
    }
}