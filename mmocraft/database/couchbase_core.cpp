#include "pch.h"
#include "couchbase_core.h"

#include <couchbase/cluster.hxx>
#include <couchbase/fmt/error.hxx>

#include "logging/logger.h"

namespace database
{
    couchbase::cluster CouchbaseCore::_global_cluster;
    std::map<database::CollectionPath, couchbase::collection> CouchbaseCore::collection_mapping;

    couchbase::cluster_options CouchbaseCore::get_cluster_option(const std::string& username, const std::string& password)
    {
        auto options = couchbase::cluster_options(username, password);
        options.timeouts()
        .query_timeout(std::chrono::seconds(10))
        .search_timeout(std::chrono::seconds(10))
        .analytics_timeout(std::chrono::seconds(10));
        return options;
    }

    bool CouchbaseCore::connect_server_with_login(const config::Configuration_Database& conf)
    {
        CONSOLE_LOG(info) << "Connecting couchbase server...";
        auto [err, cluster] = couchbase::cluster::connect(conf.server_address(), get_cluster_option(conf.userid(), conf.password())).get();

        if (err) {
            CONSOLE_LOG(error) << "Fail to connect to the cluster";
            return false;
        }

        _global_cluster = std::move(cluster);
        
        connect_collections();

        CONSOLE_LOG(info) << "Done";
        return true;
    }

    void CouchbaseCore::connect_collections()
    {
        collection_mapping.emplace(CollectionPath::player_login, _global_cluster
            .bucket(database::standard_bucket_name)
            .default_scope()
            .collection(database::player_login_collection_name));

        collection_mapping.emplace(CollectionPath::player_login_session, _global_cluster
            .bucket(database::cached_bucket_name)
            .default_scope()
            .collection(database::player_login_session_collection_name));
    }

    couchbase::cluster& CouchbaseCore::get_cluster()
    {
        return _global_cluster;
    }

    /*
    

    void MongoDBCore::connect_server(std::string_view uri)
    {
        mongoc_init();

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
    */
}