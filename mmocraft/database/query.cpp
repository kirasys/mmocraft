#include "pch.h"
#include "query.h"

#include <string.h>

#include "logging/logger.h"
#include "config/config.h"

namespace database
{
    io::DetachedTask PlayerGamedata::save(const game::Player& player_unsafe)
    {
        if (player_unsafe.is_support_gamedata_saving()) {
            auto [err, result] = co_await ::database::CouchbaseCore::upsert_document(::database::CollectionPath::player_gamedata, player_unsafe.uuid(), player_unsafe.get_gamedata());
            CONSOLE_LOG_IF(error, err) << "Fail to save player gamedata: " << err.ec();
        }
    }

    io::DetachedTask PlayerLoginSession::load(std::string_view player_name, database::collection::PlayerLoginSession& session)
    {
        auto [err, result] = co_await database::CouchbaseCore::get_document(::database::CollectionPath::player_login_session, player_name);
        if (err && err.ec() != couchbase::errc::key_value::document_not_found)
            co_return;

        if (err.ec() != couchbase::errc::key_value::document_not_found)
            session = result.content_as<database::collection::PlayerLoginSession>();
    }
}