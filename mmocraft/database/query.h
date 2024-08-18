#pragma once

#include "net/packet.h"
#include "database/sql_statement.h"

namespace database
{
    void initialize_system();

    constexpr std::size_t player_gamedata_column_size = 64;

    class PlayerLoginSQL : public SQLStatement
    {
    public:
        static constexpr const char* query = "{ call dbo.PlayerLogin(?, ?, ?, ?, ?) }";

        PlayerLoginSQL();

        bool authenticate(const char* username, const char* password);

        inline SQLUINTEGER player_identity() const
        {
            return _player_identity;
        }

        inline game::PlayerType player_type() const
        {
            return game::PlayerType(_player_type);
        }

        inline std::pair<const std::byte*, int> player_gamedata() const
        {
            return { _gamedata, sizeof(_gamedata) };
        }

    private:
        char _username[net::PacketFieldConstraint::max_username_length + 1];
        char _password[net::PacketFieldConstraint::max_password_length + 1];

        SQLUINTEGER _player_identity = 0;
        SQLUINTEGER _player_type = 0;
        std::byte _gamedata[player_gamedata_column_size];
        SQLLEN _gamedata_size = player_gamedata_column_size;
    };

    class PlayerSearchSQL : public SQLStatement
    {
    public:
        static constexpr const char* query = "SELECT id, is_admin FROM player WHERE username = ?";

        PlayerSearchSQL();

        bool search(const char* username);

        inline SQLUINTEGER player_identity() const
        {
            return _player_index;
        }

        inline bool is_admin() const
        {
            return _is_admin == 1;
        }

    private:
        char _username[net::PacketFieldConstraint::max_username_length + 1];

        SQLUINTEGER _player_index = 0;
        SQLCHAR _is_admin;
    };

    class PlayerUpdateSQL : public SQLStatement
    {
    public:
        static constexpr const char* query = "UPDATE player_game_data SET gamedata = ? WHERE player_id = ?";

        PlayerUpdateSQL();

        bool update(const game::Player&);

    private:
        SQLINTEGER player_id;
        std::byte _player_gamedata[player_gamedata_column_size];
        SQLLEN _player_gamedata_size = player_gamedata_column_size;
    };
}