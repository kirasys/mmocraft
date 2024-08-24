#include "pch.h"
#include "mongodb_core.h"

#include "logging/logger.h"

namespace
{
    constexpr const char* mongodb_database_name = "mmocraft";

    mongocxx::instance mongodb_instance{};   
}

namespace database
{
    std::unique_ptr<mongocxx::pool> MongoDBCore::connection_pool;

    void MongoDBCore::connect_server(std::string_view uri)
    {
        CONSOLE_LOG(info) << "Connecting mongodb database server...";
        mongocxx::uri connection_uri{ uri };
        connection_pool.reset(new mongocxx::pool{ connection_uri });

        get_database();
        CONSOLE_LOG(info) << "Done";
    }

    mongocxx::database& MongoDBCore::get_database()
    {
        thread_local std::unique_ptr<mongocxx::pool::entry> client;
        thread_local std::unique_ptr<mongocxx::database> db;

        if (not client) {
            client.reset(new mongocxx::pool::entry(connection_pool->acquire()));
            db.reset(new mongocxx::database((**client)[mongodb_database_name]));

            // force to connect to the server.
            (*db)["connection_test"].delete_many({});
        }

        return *db;
    }
}