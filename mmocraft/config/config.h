#pragma once

#include <string_view>

#include "net/udp_message.h"
#include "net/server_communicator.h"
#include "proto/generated/config.pb.h"

namespace config {
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

    void set_default_configuration(Configuration_Server&);
    void set_default_configuration(Configuration_World&);
    void set_default_configuration(Configuration_Database&);
    void set_default_configuration(Configuration_Log&);
    void set_default_configuration(Configuration_System&);

    void initialize_system(std::string_view config_dir, std::string_view config_filename);

    template <typename ConfigType>
    bool load_remote_config(const char* router_ip, int router_port, protocol::ServerType server_type, ConfigType& config)
    {
        net::MessageResponse response;
        if (not net::ServerCommunicator::get_config(router_ip, router_port, server_type, response)) {
            std::cerr << "Fail to get config from remote(" << router_ip << ", " << router_port << ')';
            return false;
        }

        protocol::GetConfigResponse get_config_res;
        if (not get_config_res.ParseFromArray(response.begin_message(), int(response.message_size()))) {
            std::cerr << "Fail to parse GetConfigResponse";
            return false;
        }

        if (not config.ParseFromString(get_config_res.config())) {
            std::cerr << "Fail to parse ChatConfig";
            return false;
        }

        set_default_configuration(*config.mutable_system());
        return true;
    }
}