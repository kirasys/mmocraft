#include "pch.h"
#include "query.h"

#include <string.h>

#include "logging/logger.h"
#include "config/config.h"

namespace database
{
    database::AsyncTask PlayerGamedata::save(const game::Player& player_unsafe)
    {
        if (player_unsafe.is_support_gamedata_saving()) {
            auto [err, result] = co_await ::database::CouchbaseCore::upsert_document(::database::CollectionPath::player_gamedata, player_unsafe.uuid(), player_unsafe.get_gamedata());
            CONSOLE_LOG_IF(error, err) << "Fail to save player gamedata: " << err.ec();
        }
    }
}