#pragma once

#include <string_view>

#include "net/udp_message.h"
#include "net/server_communicator.h"
#include "proto/generated/config.pb.h"
#include "proto/generated/protocol.pb.h"

namespace config {
    FrontendConfig& get_config();

    void initialize_system(std::string_view config_dir, std::string_view config_filename);

    void set_default_configuration(config::Configuration_System& conf);

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