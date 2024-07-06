#pragma once

#include "net/packet.h"
#include "database/sql_statement.h"

namespace database
{
    constexpr const char* sql_select_player_by_username_and_password = "SELECT COUNT(*) FROM player WHERE username = ? AND password = dbo.GetPasswordHash(?)";
    constexpr const char* sql_select_player_by_username = "SELECT id FROM player WHERE username = ?";
    constexpr const char* sql_select_player_game_data = "SELECT latest_position, spawn_position FROM player_game_data WHERE player_id = ?";
    constexpr const char* sql_update_player_game_data = "UPDATE player_game_data SET latest_position = ?, spawn_position = ? WHERE player_id = ?";

    class PlayerLoginSQL : public SQLStatement
    {
    public:
        PlayerLoginSQL(SQLHDBC);

        bool authenticate(const char* username, const char* password);

    private:
        char _username[net::PacketFieldConstraint::max_username_length + 1];
        char _password[net::PacketFieldConstraint::max_password_length + 1];
    };

    class PlayerSearchSQL : public SQLStatement
    {
    public:
        PlayerSearchSQL(SQLHDBC);

        bool search(const char* username);

        inline SQLUINTEGER get_player_identity() const
        {
            return player_index;
        }

    private:
        char _username[net::PacketFieldConstraint::max_username_length + 1];

        SQLUINTEGER player_index = 0;
    };

    class PlayerDataLoadSQL : public SQLStatement
    {
    public:
        PlayerDataLoadSQL(SQLHDBC);

        bool load(game::Player&);

    private:
        SQLINTEGER player_id;
        game::PlayerPosition latest_pos;
        game::PlayerPosition spawn_pos;
    };

    class PlayerUpdateSQL : public SQLStatement
    {
    public:
        PlayerUpdateSQL(SQLHDBC);

        bool update(const game::Player&);

    private:
        SQLINTEGER player_id;
        game::PlayerPosition latest_pos;
        game::PlayerPosition spawn_pos;
    };
}