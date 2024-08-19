#include "config.h"

namespace
{
    config::ChatConfig g_config;
}

namespace chat
{
namespace config
{
    ::config::ChatConfig& get_config()
    {
        return g_config;
    }
}
}