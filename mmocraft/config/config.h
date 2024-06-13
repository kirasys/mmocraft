#pragma once

namespace config {
    constexpr const char* config_file_path = "config/config.json";

    class Configuration;
    const Configuration& get_config();

    void initialize_system();
}