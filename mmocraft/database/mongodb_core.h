#pragma once

#include <memory>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/uri.hpp>

#include "util/common_util.h"

namespace database
{
    class MongoDBCore : util::NonCopyable, util::NonMovable
    {
    public:
        MongoDBCore() = default;

        ~MongoDBCore() = default;

        void connect(std::string_view uri);

        mongocxx::database& get_database();

    private:
        std::unique_ptr<mongocxx::pool> connection_pool;
    };
}