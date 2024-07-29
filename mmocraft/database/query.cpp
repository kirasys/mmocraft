#include "pch.h"
#include "query.h"

#include <string.h>

#include "database/database_core.h"
#include "database/mongodb_core.h"
#include "logging/logger.h"
#include "proto/config.pb.h"

namespace
{
    database::DatabaseCore global_database_connection;
    database::MongoDBCore global_mongodb_connection;
}

namespace database
{
    void initialize_system()
    {
        const auto& db_conf = config::get_database_config();

        // start database system.
        CONSOLE_LOG(info) << "Connecting database server...";
        if (not global_database_connection.connect_with_password(db_conf))
            throw error::DATABASE_CONNECT;
        CONSOLE_LOG(info) << "Connected";

        CONSOLE_LOG(info) << "Connecting mongodb server..";
        global_mongodb_connection.connect(db_conf.mongodb_uri());
        CONSOLE_LOG(info) << "Connected";
    }

    PlayerLoginSQL::PlayerLoginSQL()
        : SQLStatement{ global_database_connection.get_connection_handle() }
    {
        this->prepare(query);

        // bind input parameters.
        this->inbound_null_terminated_string_parameter(1, _username, sizeof(_username));
        this->inbound_null_terminated_string_parameter(2, _password, sizeof(_password));

        // bind output parameters.
        this->outbound_uint32_parameter(3, _player_identity);
        this->outbound_uint32_parameter(4, _player_type);
        this->outbound_bytes_parameter(5, _gamedata, player_gamedata_column_size, _gamedata_size);
    }

    bool PlayerLoginSQL::authenticate(const char* a_username, const char* a_password)
    {
        ::strcpy_s(_username, a_username);
        ::strcpy_s(_password, a_password);

        if (this->execute()) {
            while (this->more_results()) { }
            return true;
        }

        return false;
    }

    PlayerSearchSQL::PlayerSearchSQL()
        : SQLStatement{ global_database_connection.get_connection_handle() }
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

    PlayerUpdateSQL::PlayerUpdateSQL()
        : SQLStatement{ global_database_connection.get_connection_handle() }
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

    void MailDocument::insert(const char* message)
    {
        auto& db = global_mongodb_connection.get_database();
        //db[collection_name].insert_one();
    }
}