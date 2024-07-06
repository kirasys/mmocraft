#include "pch.h"
#include "query.h"

#include <string.h>

namespace database
{
    PlayerLoginSQL::PlayerLoginSQL(SQLHDBC a_connection_handle)
        : SQLStatement{ a_connection_handle }
    {
        this->prepare(sql_select_player_by_username_and_password);

        // bind input parameters.
        this->inbound_null_terminated_string_parameter(1, _username, sizeof(_username));
        this->inbound_null_terminated_string_parameter(2, _password, sizeof(_password));
    }

    bool PlayerLoginSQL::authenticate(const char* a_username, const char* a_password)
    {
        ::strcpy_s(_username, a_username);
        ::strcpy_s(_password, a_password);

        if (this->execute()) {
            util::defer clear_cursor = [this] { this->close_cursor(); };
            return this->fetch();
        }

        return false;
    }

    PlayerSearchSQL::PlayerSearchSQL(SQLHDBC a_connection_handle)
        : SQLStatement{ a_connection_handle }
    {
        this->prepare(sql_select_player_by_username);

        // bind input parameters.
        this->inbound_null_terminated_string_parameter(1, _username, sizeof(_username));

        // bind output parameters.
        this->outbound_unsigned_integer_column(1, player_index);
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

    PlayerUpdateSQL::PlayerUpdateSQL(SQLHDBC a_connection_handle)
        : SQLStatement{ a_connection_handle }
    {
        this->prepare(sql_update_player_by_id);

        // bind input parameters.
        this->inbound_uint64_parameter(1, latest_pos.raw);
        this->inbound_uint64_parameter(2, spawn_pos.raw);
        this->inbound_int32_parameter(3, player_id);
    }

    bool PlayerUpdateSQL::update(const game::Player& player)
    {
        // set input parameters.
        latest_pos = player.last_position();
        spawn_pos = player.spawn_position();
        player_id = player.identity();

        return this->execute();
    }
}