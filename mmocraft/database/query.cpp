#include "pch.h"
#include "query.h"

#include <string.h>

#include "database/database_core.h"
#include "logging/logger.h"
#include "config/config.h"

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;

namespace
{
    database::DatabaseCore msdb_core;
}

namespace database
{
    PlayerSearchSQL::PlayerSearchSQL()
        : SQLStatement{ msdb_core.get_connection() }
    {
        this->prepare(query);

        // bind input parameters.
        this->inbound_null_terminated_string_parameter(1, _username, sizeof(_username));

        // bind output parameters.
        this->outbound_uint32_column(1, _player_index);
        this->outbound_bool_column(2, _is_admin);
    }

    bool PlayerSearchSQL::search(const char* a_username)
    {
        ::strcpy_s(_username, a_username);

        if (this->execute()) {
            util::defer clear_cursor = [this] { this->close_cursor(); };
            return this->fetch();
        }

        return false;
    }

    PlayerLoadSQL::PlayerLoadSQL()
        : SQLStatement{ msdb_core.get_connection() }
    {
        this->prepare(query);

        // bind input parameters.
        this->inbound_int32_parameter(1, player_id);

        // bind output parameters.
        this->outbound_bytes_column(1, _gamedata, sizeof(_gamedata), _gamedata_size);
    }
    
    bool PlayerLoadSQL::load(game::Player& player)
    {
        // set input parameters.
        player_id = player.identity();

        if (this->execute()) {
            util::defer clear_cursor = [this] { this->close_cursor(); };
            if (this->fetch()) {
                player.load_gamedata(_gamedata, sizeof(_gamedata));
                return true;
            }
        }

        return false;
    }

    PlayerUpdateSQL::PlayerUpdateSQL()
        : SQLStatement{ msdb_core.get_connection() }
    {
        this->prepare(query);

        // bind input parameters.
        this->inbound_bytes_parameter(1, _player_gamedata, player_gamedata_column_size, _player_gamedata_size);
        this->inbound_int32_parameter(2, player_id);
    }

    bool PlayerUpdateSQL::update(const game::Player& player)
    {
        if (player.player_type() < game::PlayerType::AUTHENTICATED_USER)
            return true;

        // set input parameters.
        player_id = player.identity();

        memset(_player_gamedata, 0, sizeof(_player_gamedata));
        player.copy_gamedata(_player_gamedata, sizeof(_player_gamedata));

        return this->execute();
    }

    PlayerSession::PlayerSession(std::string_view username)
        : _username{ username }
    {
        find(username);
    }

    bool PlayerSession::find(std::string_view username)
    {
        auto collection = database::MongoDBCore::get_database().collection(collection_name);
        _cursor = std::move(collection.find_one(make_document(kvp("username", username))));
        return _cursor.has_value();
    }

    bool PlayerSession::update(net::ConnectionKey connection_key, game::PlayerType player_type, unsigned player_identity)
    {
        auto collection = database::MongoDBCore::get_database().collection(collection_name);

        auto doc = bsoncxx::builder::basic::document{};
        doc.append(
            kvp("connection_key", bsoncxx::types::b_int64(connection_key.raw())),
            kvp("player_type", bsoncxx::types::b_int32(player_type)),
            kvp("player_identity", bsoncxx::types::b_int32(player_identity)),
            kvp("expired_at", bsoncxx::types::b_date(std::chrono::system_clock::now() + expiration_period))
        );
        
        if (exists()) {
            auto update_one_result = collection.update_one(
                make_document(kvp("username", _username)),
                make_document(kvp("$set", doc.view()))
            );
            return update_one_result->modified_count() == 1;
        }
        else {
            doc.append(kvp("username", _username));
            return collection.insert_one(doc.view()).has_value();
        }
    }

    bool PlayerSession::revoke()
    {
        auto collection = database::MongoDBCore::get_database().collection(collection_name);
        auto update_one_result = collection.update_one(
            make_document(kvp("username", _username)),
            make_document(kvp("$set", 
                make_document(kvp("expired_at", bsoncxx::types::b_date(std::chrono::system_clock::now()))))
            )
        );
        return update_one_result->modified_count() == 1;
    }

    void PlayerSession::create_collection()
    {
        auto& db = database::MongoDBCore::get_database();
        db.create_collection(collection_name);

        auto coll = db[collection_name];

        {
            mongocxx::options::index index_options{};
            index_options.unique(true);

            coll.create_index(make_document(kvp("username", 1)), index_options);
        }

        {
            mongocxx::options::index index_options{};
            index_options.expire_after(std::chrono::seconds(0));

            coll.create_index(make_document(kvp("expired_at", 1)), index_options);
        }
    }
}