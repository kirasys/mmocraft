#pragma once

#include <proto/generated/config.pb.h>

namespace chat
{
    namespace config
    {
        ::config::ChatConfig& get_config();
    }
}