#include "query.h"

#include <config/constants.h>
#include <database/database_core.h>
#include <database/couchbase_core.h>
#include <logging/logger.h>
#include <util/string_util.h>

#include "../config/config.h"

namespace login
{
namespace database
{
    PlayerLoginSQL::PlayerLoginSQL()
        : SQLStatement{ ::database::DatabaseCore::get_connection() }
    {
        this->prepare(query);

        // bind input parameters.
        this->inbound_null_terminated_string_parameter(1, _username, sizeof(_username));
        this->inbound_null_terminated_string_parameter(2, _password, sizeof(_password));

        // bind output parameters.
        this->outbound_uint32_parameter(3, _player_identity);
        this->outbound_uint32_parameter(4, _player_type);
    }

    bool PlayerLoginSQL::authenticate(std::string_view a_username, std::string_view a_password)
    {
        util::string_copy(_username, a_username);
        util::string_copy(_password, a_password);

        if (this->execute()) {
            while (this->more_results()) {}
            return true;
        }

        return false;
    }

    PlayerSession::PlayerSession(std::string_view username)
        : _username{ username }
    {
        find();
    }

    bool PlayerSession::find()
    {
        /*
        auto collection = database::MongoDBCore::get_database().collection(collection_name);
        _cursor = std::move(collection.find_one(make_document(kvp("username", username))));
        return _cursor.has_value();
        */
        return true;
    }

    bool PlayerSession::update(net::ConnectionKey connection_key, game::PlayerType player_type, unsigned player_identity)
    {
        /*
        auto collection = database::MongoDBCore::get_database().collection(collection_name);

        auto doc = bsoncxx::builder::basic::document{};
        doc.append(
            kvp("connection_key", bsoncxx::types::b_int64(connection_key.raw())),
            kvp("player_type", bsoncxx::types::b_int32(player_type)),
            kvp("player_identity", bsoncxx::types::b_int32(player_identity)),
            kvp("expired_at", bsoncxx::types::b_date(std::chrono::system_clock::now() + session_max_lifetime))
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
        */
        return true;
    }

    bool PlayerSession::revoke()
    {
        /*
        auto collection = database::MongoDBCore::get_database().collection(collection_name);
        auto update_one_result = collection.update_one(
            make_document(kvp("username", _username)),
            make_document(kvp("$set",
                make_document(
                    kvp("expired_at", bsoncxx::types::b_date(std::chrono::system_clock::now() + expiration_period)))
                )
            )
        );
        */
        return true;
    }
}
}
