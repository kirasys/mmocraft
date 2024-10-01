#include "pch.h"
#include "query.h"

#include <string.h>

#include "database/database_core.h"
#include "logging/logger.h"
#include "config/config.h"

namespace
{
    database::DatabaseCore msdb_core;
}

namespace database
{
    PlayerSearchSQL::PlayerSearchSQL()
        : SQLStatement{ msdb_core.get_connection() }
    {
        this->prepare(query);

        // bind input parameters.
        this->inbound_null_terminated_string_parameter(1, _username, sizeof(_username));

        // bind output parameters.
        this->outbound_uint32_column(1, _player_index);
        this->outbound_bool_column(2, _is_admin);
    }

    bool PlayerSearchSQL::search(const char* a_username)
    {
        ::strcpy_s(_username, a_username);

        if (this->execute()) {
            util::defer clear_cursor = [this] { this->close_cursor(); };
            return this->fetch();
        }

        return false;
    }

    PlayerLoadSQL::PlayerLoadSQL()
        : SQLStatement{ msdb_core.get_connection() }
    {
        this->prepare(query);

        // bind input parameters.
        this->inbound_int32_parameter(1, player_id);

        // bind output parameters.
        this->outbound_bytes_column(1, _gamedata, sizeof(_gamedata), _gamedata_size);
    }
    
    bool PlayerLoadSQL::load(game::Player& player)
    {
        // set input parameters.
        player_id = player.identity();

        if (this->execute()) {
            util::defer clear_cursor = [this] { this->close_cursor(); };
            if (this->fetch()) {
                player.load_gamedata(_gamedata, sizeof(_gamedata));
                return true;
            }
        }

        return false;
    }

    PlayerUpdateSQL::PlayerUpdateSQL()
        : SQLStatement{ msdb_core.get_connection() }
    {
        this->prepare(query);

        // bind input parameters.
        this->inbound_bytes_parameter(1, _player_gamedata, player_gamedata_column_size, _player_gamedata_size);
        this->inbound_int32_parameter(2, player_id);
    }

    bool PlayerUpdateSQL::update(const game::Player& player)
    {
        if (player.player_type() < game::PlayerType::AUTHENTICATED_USER)
            return true;

        // set input parameters.
        player_id = player.identity();

        memset(_player_gamedata, 0, sizeof(_player_gamedata));
        player.copy_gamedata(_player_gamedata, sizeof(_player_gamedata));

        return this->execute();
    }
}