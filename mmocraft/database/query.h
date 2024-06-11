#pragma once

#include "net/packet.h"
#include "database/sql_statement.h"

namespace database
{
    constexpr const char* sql_select_player_by_username_and_password = "SELECT COUNT(*) FROM player WHERE username = ? AND password = dbo.GetPasswordHash(?)";
    constexpr const char* sql_select_player_by_username = "SELECT id FROM player WHERE username = ?";

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
}