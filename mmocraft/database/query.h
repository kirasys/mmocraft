#pragma once

#include "net/packet.h"
#include "net/connection_environment.h"
#include "database/sql_statement.h"
#include "database/couchbase_core.h"

namespace database
{
    constexpr std::size_t player_gamedata_column_size = 64;

    class PlayerGamedata
    {
    public:

         static io::AsyncTask<void> save(const game::Player& player_unsafe);

    };

    class PlayerLoginSession
    {
    public:

        static io::AsyncTask<void> load(std::string_view player_name, database::collection::PlayerLoginSession&);

    };
}