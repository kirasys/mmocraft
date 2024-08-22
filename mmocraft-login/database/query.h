#pragma once

#include <net/packet.h>
#include <database/sql_statement.h>

namespace login
{
    constexpr std::size_t player_gamedata_column_size = 64;

    namespace database
    {
        void initialize_system();

        class PlayerLoginSQL : public ::database::SQLStatement
        {
        public:
            static constexpr const char* query = "{ call dbo.PlayerLogin(?, ?, ?, ?) }";

            PlayerLoginSQL();

            bool authenticate(std::string_view username, std::string_view password);

            inline SQLUINTEGER player_identity() const
            {
                return _player_identity;
            }

            inline game::PlayerType player_type() const
            {
                return game::PlayerType(_player_type);
            }

        private:
            char _username[::net::PacketFieldConstraint::max_username_length + 1];
            char _password[::net::PacketFieldConstraint::max_password_length + 1];

            SQLUINTEGER _player_identity = 0;
            SQLUINTEGER _player_type = 0;
        };
    }
}