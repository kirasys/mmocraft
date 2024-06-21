#include "pch.h"
#include "config.h"

#include <iostream>
#include <string>
#include <filesystem>
#include <google/protobuf/util/json_util.h>

#include "logging/error.h"
#include "logging/logger.h"
#include "proto/config.pb.h"
#include "util/protobuf_util.h"

namespace fs = std::filesystem;

namespace {
    config::Configuration g_configuration;
}

namespace config {
    void set_default_configuration()
    {
        // server default values
        auto& server_conf = get_server_config();
        server_conf.set_ip("127.0.0.1");
        server_conf.set_port(12345);
        server_conf.set_max_player(1000);
        server_conf.set_server_name("Massive Minecraft Classic Server");
        server_conf.set_motd("welcome to mmocraft server.");

        // world default values
        auto& world_conf = get_world_config();
        world_conf.set_width(256);
        world_conf.set_height(256);
        world_conf.set_length(256);
        world_conf.set_save_dir("world\\");

        // log default values
        auto& log_conf = get_log_config();
        log_conf.set_log_file_path("server.log");

        // database default values
        auto& database_conf = get_database_config();
        database_conf.set_driver_name("ODBC Driver 17 for SQL Server");
        database_conf.set_database_name("mmocraft");
        database_conf.set_userid("mmocraft_login");
        database_conf.set_password("12341234");
    }

    void set_system_configuration()
    {
        SYSTEM_INFO sys_info;
        ::GetSystemInfo(&sys_info);

        auto& system_conf = get_system_config();
        system_conf.set_page_size(sys_info.dwPageSize);
        system_conf.set_alllocation_granularity(sys_info.dwAllocationGranularity);
        system_conf.set_num_of_processors(sys_info.dwNumberOfProcessors);
    }

    void generate_config()
    {
        set_default_configuration();

        util::proto_message_to_json_file(g_configuration, config_file_path);
    }

    void load_config()
    {
        if (not fs::exists(config_file_path)) {
            generate_config();
            CONSOLE_LOG(fatal) << "Configuration file is generated at \"" << config_file_path << "\". "
                << "Please fill in appropriate values.";
        }

        util::json_file_to_proto_message(&g_configuration, config_file_path);

        set_system_configuration();
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
        return *g_configuration.mutable_system();
    }

    void initialize_system()
    {
        load_config();
    }
}