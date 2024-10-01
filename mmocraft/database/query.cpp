#include "pch.h"
#include "query.h"

#include <string.h>

#include "logging/logger.h"
#include "config/config.h"

namespace database
{
    database::AsyncTask PlayerGamedata::load(net::ConnectionEnvironment& connection_env, const game::Player& player_unsafe)
    {
        auto connection_key = player_unsafe.connection_key();
        auto [err, result] = co_await ::database::CouchbaseCore::get_document(::database::CollectionPath::player_gamedata, player_unsafe.uuid());

        if (auto conn = connection_env.try_acquire_connection(connection_key)) {
            if (err) {
                conn->kick(error::PACKET_RESULT_FAIL_LOGIN);
            }

            // Note: don't use player_unsafe after resume().
            else if (auto player = conn->associated_player()) {
                player->set_gamedata(result.content_as<::database::collection::PlayerGamedata>());
                player->set_state(game::PlayerState::ExHandshake_Completed);
            }
        }

        co_return;
            
    }

    database::AsyncTask PlayerGamedata::save(const game::Player& player_unsafe)
    {
        auto [err, result] = co_await ::database::CouchbaseCore::upsert_document(::database::CollectionPath::player_gamedata, player_unsafe.uuid(), player_unsafe.get_gamedata());
        CONSOLE_LOG_IF(error, err) << "Fail to save player gamedata: " << err.ec();
    }
}