#include "config.h"

namespace
{
    config::LoginConfig g_config;
}

namespace login
{
    namespace config
    {
        ::config::LoginConfig& get_config()
        {
            return g_config;
        }
    }
}