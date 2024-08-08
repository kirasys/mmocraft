#include "config.h"

#include <array>
#include <filesystem>
#include <logging/logger.h>
#include <proto/generated/config.pb.h>
#include <util/protobuf_util.h>

namespace fs = std::filesystem;

namespace
{
    ::config::RouterConfig g_route_server_config;
    ::config::ChatConfig g_chat_server_config;

    struct ConfigEntry
    {
        protocol::ServerType server_type;
        const char* config_path = nullptr;
        google::protobuf::Message* config_msg = nullptr;
    };

    constinit const std::array<ConfigEntry, 0xff> config_path_db = [] {
        std::array<ConfigEntry, 0xff> arr{};
        arr[protocol::ServerType::Router] = { protocol::ServerType::Router, "config/router_config.json", &g_route_server_config };
        arr[protocol::ServerType::Chat] = { protocol::ServerType::Chat, "config/chat_config.json", &g_chat_server_config };
        return arr;
    }();

    // Note: proto3 does not print default-initialized fields.
    //       To print all fields, we invoke mutable method explicitly.
    void set_default_configuration(protocol::ServerType server_type)
    {
        switch (server_type) {
        case protocol::ServerType::Router:
            g_route_server_config.mutable_server();
            g_route_server_config.mutable_log();
            return;
        case protocol::ServerType::Chat:
            g_chat_server_config.mutable_server();
            g_chat_server_config.mutable_rel_database();
            g_chat_server_config.mutable_doc_database();
            g_chat_server_config.mutable_log();
            return;
        }
    }

    void generate_config(const char* config_path, google::protobuf::Message* msg)
    {
        util::proto_message_to_json_file(*msg, config_path);
    }

    void generate_all_config()
    {
        bool config_file_created = false;

        for (auto [server_type, config_path, config_msg] : config_path_db) {
            if (config_path && not fs::exists(config_path)) {
                set_default_configuration(server_type);
                generate_config(config_path, config_msg);
                CONSOLE_LOG(info) << "Configuration file is generated at \"" << config_path << "\". "
                    << "Please fill in appropriate values.";
                config_file_created = true;
            }
        }

        if (config_file_created)
            std::exit(0);
    }
}

namespace router {
namespace config {

    void initialize_system()
    {
        generate_all_config();

        // Load route server config.
        load_server_config(protocol::ServerType::Router);
    }

    ::config::Configuration_Server& get_server_config()
    {
        return *g_route_server_config.mutable_server();
    }

    ::config::Configuration_Log& get_log_config()
    {
        return *g_route_server_config.mutable_log();
    }

    bool load_server_config(protocol::ServerType server_type, protocol::GetConfigResponse* msg)
    {
        auto [_, config_path, config_msg] = config_path_db[server_type];
        if (config_path == nullptr) {
            CONSOLE_LOG(error) << "Unsupported server type : " << server_type;
            return false;
        }

        util::json_file_to_proto_message(config_msg, config_path);
        if (msg) msg->set_config(config_msg->SerializeAsString());
        return true;
    }

}
}