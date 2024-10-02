#pragma once

#include <proto/generated/config.pb.h>
#include <proto/generated/protocol.pb.h>

namespace router
{
    namespace config
    {
        void initialize_system();

        bool load_server_config(protocol::server_type_id, protocol::FetchConfigResponse* msg = nullptr);

        ::config::Configuration_Server& get_server_config();

        ::config::Configuration_Log& get_log_config();
    }
}