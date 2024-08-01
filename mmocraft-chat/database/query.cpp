#include "query.h"

#include <database/mongodb_core.h>

#include "logging/logger.h"

namespace
{
    database::MongoDBCore global_mongodb_connection;
}

namespace database
{
    void connect_mongodb_server(std::string_view uri)
    {
        CONSOLE_LOG(info) << "Connecting mongodb server..";
        global_mongodb_connection.connect(uri);
        CONSOLE_LOG(info) << "Connected";
    }

    void MailDocument::insert(const char* message)
    {
        auto& db = global_mongodb_connection.get_database();
        //db[collection_name].insert_one();

    }
}