#include "query.h"

#include <database/database_core.h>
#include <logging/logger.h>
#include <util/string_util.h>

#include "../config/config.h"

namespace
{
    database::DatabaseCore global_database_connection;
}

namespace login
{
namespace database
{
    void initialize_system()
    {
        const auto& conf = config::get_config();

        if (not ::database::connect_database_server(&global_database_connection, conf.rel_database()))
            throw error::DATABASE_CONNECT;
    }

    PlayerLoginSQL::PlayerLoginSQL()
        : SQLStatement{ global_database_connection.get_connection_handle() }
    {
        this->prepare(query);

        // bind input parameters.
        this->inbound_null_terminated_string_parameter(1, _username, sizeof(_username));
        this->inbound_null_terminated_string_parameter(2, _password, sizeof(_password));

        // bind output parameters.
        this->outbound_uint32_parameter(3, _player_identity);
        this->outbound_uint32_parameter(4, _player_type);
    }

    bool PlayerLoginSQL::authenticate(std::string_view a_username, std::string_view a_password)
    {
        util::string_copy(_username, a_username);
        util::string_copy(_password, a_password);

        if (this->execute()) {
            while (this->more_results()) {}
            return true;
        }

        return false;
    }
}
}
