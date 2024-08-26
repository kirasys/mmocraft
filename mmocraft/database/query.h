#pragma once

#include "net/packet.h"
#include "database/sql_statement.h"
#include "database/mongodb_core.h"

namespace database
{
    constexpr std::size_t player_gamedata_column_size = 64;

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

    class PlayerLoadSQL : public SQLStatement
    {
    public:
        static constexpr const char* query = "SELECT gamedata FROM player_game_data WHERE player_id = ?";

        PlayerLoadSQL();

        bool load(game::Player&);

    private:
        SQLINTEGER player_id;
        std::byte _gamedata[player_gamedata_column_size];
        SQLLEN _gamedata_size = player_gamedata_column_size;
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

    class PlayerSession
    {
    public:
        static constexpr const char* collection_name = "player_session";

        static constexpr std::chrono::days expiration_period{ 30 };

        PlayerSession(std::string_view username);

        bool exists() const
        {
            return _cursor.has_value();
        }

        auto connection_key() const
        {
            return net::ConnectionKey((*_cursor)["connection_key"].get_int64());
        }

        bool update(net::ConnectionKey, game::PlayerType, unsigned);

        bool revoke();

        static void create_collection();

    private:
        bool find(std::string_view username);

        std::string_view _username;

        std::optional<bsoncxx::document::value> _cursor;
    };
}