#pragma once

#include <coroutine>
#include <memory>
#include <map>

#include <couchbase/cluster.hxx>
#include <tao/json.hpp>

#include "database/couchbase_definitions.h"
#include "proto/generated/config.pb.h"
#include "util/common_util.h"
#include "util/uuid_v4.h"

namespace database
{
    template <typename T>
    struct AsyncTask
    {
        struct promise_type
        {
            AsyncTask get_return_object() { return {}; }
            std::suspend_never initial_suspend() { return {}; }
            std::suspend_never final_suspend() noexcept { return {}; }

            void return_value(T&& value)
            {
                _value = std::forward(value);
            }

            T result()
            {
                return _value;
            }

            void unhandled_exception() {}

        private:
            T _value;
        };

        auto operator co_await()
        {
            struct awaitable
            {
                constexpr bool await_ready() const noexcept { return false; }

                void await_suspend(std::coroutine_handle<> coro)
                {

                }

                T await_resume() {}
            };
            return awaitable{};
        }
    };

    template<>
    struct AsyncTask<void>
    {
        struct promise_type
        {
            AsyncTask get_return_object() { return {}; }
            std::suspend_never initial_suspend() { return {}; }
            std::suspend_never final_suspend() noexcept { return {}; }
            void return_void() {}
            void unhandled_exception() {}
        };
    };

    class CouchbaseCore : util::NonCopyable, util::NonMovable
    {
    public:
        CouchbaseCore() = default;

        ~CouchbaseCore() = default;

        static bool connect_server_with_login(const config::Configuration_Database& conf);

        static couchbase::cluster_options get_cluster_option(const std::string& username, const std::string& password);

        static couchbase::cluster& get_cluster();

        static couchbase::collection& get_collection(database::CollectionPath path)
        {
            return collection_mapping.at(path);
        }

        struct DataOperationAwaiter
        {
            constexpr bool await_ready() const noexcept { return false; }

            auto await_resume() const noexcept -> std::pair<couchbase::error, couchbase::get_result>
            {
                return { std::move(error), std::move(result) };
            }

            DataOperationAwaiter(database::CollectionPath path, std::string_view name)
                : collection_path{ path }
                , document_name{ name }
            { }

            DataOperationAwaiter(database::CollectionPath path, std::string_view name, couchbase::codec::encoded_value body)
                : collection_path{ path }
                , document_name{ name }
                , document_body{ body }
            { }

            inline std::string document_id() const
            {
                return std::string(document_name) + ':' + to_string(collection_path);
            }

            database::CollectionPath collection_path;
            std::string_view document_name;

            couchbase::codec::encoded_value document_body;

            couchbase::error error;
            couchbase::get_result result;
        };

        struct GetOperationAwaiter : DataOperationAwaiter
        {
            using DataOperationAwaiter::DataOperationAwaiter;

            void await_suspend(std::coroutine_handle<> coro)
            {
                auto& coll = CouchbaseCore::get_collection(collection_path);
                coll.get(document_id(), {}, [this, coro](auto err, auto res) {
                    error = std::move(err);
                    result = std::move(res);
                    coro.resume();
                });
            }
        };

        struct UpsertOperationAwaiter : DataOperationAwaiter
        {
            using DataOperationAwaiter::DataOperationAwaiter;

            void await_suspend(std::coroutine_handle<> coro)
            {
                auto& coll = CouchbaseCore::get_collection(collection_path);
                coll.upsert(document_id(), std::move(document_body), {}, [this, coro](auto err, auto&& res) {
                    error = std::move(err);
                    coro.resume();
                });
            }
        };

        struct DeleteOperationAwaiter : DataOperationAwaiter
        {
            using DataOperationAwaiter::DataOperationAwaiter;

            void await_suspend(std::coroutine_handle<> coro)
            {
                auto& coll = CouchbaseCore::get_collection(collection_path);
                coll.remove(document_id(), {}, [this, coro](auto err, auto&& res) {
                    error = std::move(err);
                    coro.resume();
                });
            }
        };

        struct InsertOperationAwaiter : DataOperationAwaiter
        {
            using DataOperationAwaiter::DataOperationAwaiter;

            void await_suspend(std::coroutine_handle<> coro)
            {
                auto& coll = CouchbaseCore::get_collection(collection_path);
                coll.insert(util::uuid(), std::move(document_body), {}, [this, coro](auto err, auto&& res) {
                    error = std::move(err);
                    coro.resume();
                });
            }
        };

        static GetOperationAwaiter get_document(database::CollectionPath path, std::string_view name)
        {
            return { path, name };
        }

        template<typename Transcoder = couchbase::codec::default_json_transcoder, typename Document>
        static UpsertOperationAwaiter upsert_document(database::CollectionPath path, std::string_view name, const Document& document)
        {
            return { path, name, Transcoder::encode(document) };
        }

        static DeleteOperationAwaiter remove_document(database::CollectionPath path, std::string_view name)
        {
            return { path, name };
        }

        template<typename Transcoder = couchbase::codec::default_json_transcoder, typename Document>
        static InsertOperationAwaiter insert_document(database::CollectionPath path, const Document& document)
        {
            return { path, "", Transcoder::encode(document) };
        }

    private:

        static void connect_collections();

        static void connect_collection(const char* bucket_name, database::CollectionPath path);

        static couchbase::cluster _global_cluster;

        static std::map<database::CollectionPath, couchbase::collection> collection_mapping;
    };
}