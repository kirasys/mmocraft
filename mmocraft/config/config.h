#pragma once

#include <string_view>

#include "proto/generated/config.pb.h"

namespace config {
    config::FrontendConfig& get_config();

    void initialize_system(std::string_view config_dir, std::string_view config_filename);

    void set_default_configuration(config::Configuration_System& conf);
}