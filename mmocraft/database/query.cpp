#include "pch.h"
#include "query.h"

#include <string.h>

namespace database
{
    PlayerLoginSQL::PlayerLoginSQL(SQLHDBC a_connection_handle)
        : SQLStatement{ a_connection_handle }
    {
        this->prepare(sql_login_procedure);

        // bind input parameters.
        this->inbound_null_terminated_string_parameter(1, _username, sizeof(_username));
        this->inbound_null_terminated_string_parameter(2, _password, sizeof(_password));

        // bind output parameters.
        this->outbound_uint32_parameter(3, _player_identity);
        this->outbound_uint32_parameter(4, _player_type);
        this->outbound_bytes_parameter(5, _gamedata, player_gamedata_column_size, _gamedata_size);
    }

    bool PlayerLoginSQL::authenticate(const char* a_username, const char* a_password)
    {
        ::strcpy_s(_username, a_username);
        ::strcpy_s(_password, a_password);

        if (this->execute()) {
            while (this->more_results()) { }
            return true;
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

    PlayerDataLoadSQL::PlayerDataLoadSQL(SQLHDBC a_connection_handle)
        : SQLStatement{ a_connection_handle }
    {
        this->prepare(sql_select_player_game_data);

        // bind input parameters.
        this->inbound_int32_parameter(1, player_id);

        // bind output parameters.
        this->outbound_uint64_column(1, latest_pos.raw);
        this->outbound_uint64_column(1, spawn_pos.raw);
    }

    bool PlayerDataLoadSQL::load(game::Player& player)
    {
        if (player.player_type() < game::PlayerType::AUTHENTICATED_USER)
            return true;

        player_id = player.identity();

        if (this->execute()) {
            util::defer clear_cursor = [this] { this->close_cursor(); };
            if (auto success = this->fetch()) {
                player.set_position(latest_pos);
                player.set_spawn_position(spawn_pos);
                return true;
            }
        }

        return false;
    }


    PlayerUpdateSQL::PlayerUpdateSQL(SQLHDBC a_connection_handle)
        : SQLStatement{ a_connection_handle }
    {
        this->prepare(sql_update_player_game_data);

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
        player.copy_gamedata(_player_gamedata, sizeof(_player_gamedata));

        return this->execute();
    }
}