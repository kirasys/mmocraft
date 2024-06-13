#include "pch.h"
#include "config.h"

#include <iostream>
#include <string>
#include <filesystem>
#include <google/protobuf/util/json_util.h>

#include "logging/error.h"
#include "logging/logger.h"
#include "proto/config.pb.h"

namespace {
    config::Configuration g_configuration;
}

namespace config {
    void set_default_configuration()
    {
        // server default values
        g_configuration.set_server_ip("127.0.0.1");
        g_configuration.set_server_port(12345);
        g_configuration.set_server_max_player(1000);
        g_configuration.set_server_name("Massive Minecraft Classic Server");
        g_configuration.set_server_motd("welcome to mmocraft server.");

        // world default values
        g_configuration.set_world_width(1024);
        g_configuration.set_world_height(256);
        g_configuration.set_world_length(1024);
        g_configuration.set_world_save_dir("world\\");

        // log default values
        g_configuration.set_log_file_path("server.log");

        // database default values
        g_configuration.set_database_driver_name("ODBC Driver 17 for SQL Server");
        g_configuration.set_database_name("mmocraft");
        g_configuration.set_database_userid("mmocraft_login");
        g_configuration.set_database_password("12341234");
    }

    void set_system_configuration()
    {
        SYSTEM_INFO sys_info;
        ::GetSystemInfo(&sys_info);

        g_configuration.set_system_page_size(sys_info.dwPageSize);
        g_configuration.set_system_alllocation_granularity(sys_info.dwAllocationGranularity);
        g_configuration.set_system_num_of_processors(sys_info.dwNumberOfProcessors);
    }

    void generate_config()
    {
        set_default_configuration();

        google::protobuf::util::JsonPrintOptions options;
        options.add_whitespace = true;
        options.always_print_primitive_fields = true;
        options.preserve_proto_field_names = true;

        std::string config_json;
        google::protobuf::util::MessageToJsonString(g_configuration, &config_json, options);

        std::ofstream config_file(config_file_path);
        CONSOLE_LOG_IF(fatal, config_file.fail()) << "Fail to create config file at \"" << config_file_path << '"';

        config_file << config_json << std::endl;
    }

    void load_config()
    {
        if (not std::filesystem::exists(config_file_path)) {
            generate_config();
            CONSOLE_LOG(fatal) << "Configuration file is generated at \"" << config_file_path << "\". "
                << "Please fill in appropriate values.";
        }

        std::ifstream config_file(config_file_path);
        std::string config_json((std::istreambuf_iterator<char>(config_file)), std::istreambuf_iterator<char>());
        google::protobuf::util::JsonStringToMessage(config_json, &g_configuration);

        set_system_configuration();
    }

    const Configuration& get_config()
    {
        return g_configuration;
    }

    void initialize_system()
    {
        load_config();
    }
}