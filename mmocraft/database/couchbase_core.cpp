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
        CONSOLE_LOG_IF(fatal, err) << "Fail to connect to the cluster";

        _global_cluster = std::move(cluster);
        
        connect_collections();

        CONSOLE_LOG(info) << "Done";
        return true;
    }

    void CouchbaseCore::connect_collections()
    {
        connect_collection(database::standard_bucket_name, CollectionPath::player_login);
        connect_collection(database::standard_bucket_name, CollectionPath::player_gamedata);
        connect_collection(database::cached_bucket_name, CollectionPath::player_login_session);
    }

    void CouchbaseCore::connect_collection(const char* bucket_name, database::CollectionPath path)
    {
        collection_mapping.emplace(path, _global_cluster
            .bucket(bucket_name)
            .default_scope()
            .collection(to_string(path)));
    }

    couchbase::cluster& CouchbaseCore::get_cluster()
    {
        return _global_cluster;
    }
}