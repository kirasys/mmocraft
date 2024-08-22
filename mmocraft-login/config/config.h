#pragma once

#include <proto/generated/config.pb.h>

namespace login
{
    namespace config
    {
        ::config::LoginConfig& get_config();
    }
}