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
        static database::AsyncTask load(net::ConnectionEnvironment& connection_env, const game::Player& player_unsafe);

        static database::AsyncTask get(const game::Player& player_unsafe, std::function<void(std::error_code, database::collection::PlayerGamedata)> callback);

        static database::AsyncTask save(const game::Player& player_unsafe);
    };
}