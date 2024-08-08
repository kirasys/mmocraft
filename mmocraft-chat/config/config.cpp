#include "config.h"

#include <config/config.h>
#include <net/server_communicator.h>
#include <proto/generated/config.pb.h>
#include <proto/generated/protocol.pb.h>
#include <logging/logger.h>

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