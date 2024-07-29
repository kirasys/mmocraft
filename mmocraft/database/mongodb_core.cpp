#include "pch.h"
#include "mongodb_core.h"

#include "config/config.h"
#include "proto/config.pb.h"
#include "logging/logger.h"

namespace
{
    constexpr const char* mongodb_database_name = "mmocraft";

    mongocxx::instance mongodb_instance{};
}

namespace database
{
    void MongoDBCore::connect(std::string_view uri)
    {
        mongocxx::uri connection_uri{ uri };
        connection_pool.reset(new mongocxx::pool{ connection_uri });

        get_database();
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