#pragma once

namespace config {
    constexpr const char* config_dir = "config/";
    constexpr const char* config_file_path = "config/config.json";

    class Configuration;
    class Configuration_Server;
    class Configuration_World;
    class Configuration_Database;
    class Configuration_Log;
    class Configuration_System;

    Configuration& get_config();
    Configuration_Server& get_server_config();
    Configuration_World& get_world_config();
    Configuration_Database& get_database_config();
    Configuration_Log& get_log_config();
    Configuration_System& get_system_config();

    void set_default_configuration();
    void initialize_system();
}