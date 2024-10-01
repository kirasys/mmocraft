#pragma once

#include <string>
#include <tao/json.hpp>
#include <tao/pegtl.hpp>
#include <couchbase/codec/tao_json_serializer.hxx>

namespace database
{
    // Bucket names
    constexpr const char* standard_bucket_name = "standard_bucket";
    constexpr const char* cached_bucket_name = "cached_bucket";

    // Collection names
    constexpr const char* player_login_collection_name = "player_login";
    constexpr const char* player_login_session_collection_name = "player_login_session";

    // Note: assigned value should never be changed without a migration.
    enum CollectionPath
    {
        // standard_bucket._default
        player_login = 0,

        // cached_bucket._default
        player_login_session = 1,

        SIZE,
    };

    constexpr std::string to_string(CollectionPath path)
    {
        constexpr const char* collection_path_strs[] = {
            "player_login",
            "player_login_session"
        };
        return collection_path_strs[path];
    }

    namespace collection
    {
        struct PlayerLogin
        {
            std::string username;
            std::string password;
            std::string identity;
            int player_type = 0;
        };

        struct PlayerLoginSession
        {
            std::uint64_t connection_key = 0;
        };
    }
}

template<>
struct tao::json::traits<database::collection::PlayerLogin> {
    template<template<typename...> class Traits>
    static void assign(tao::json::basic_value<Traits>& v, const database::collection::PlayerLogin& p)
    {
        v = {
            { "username", p.username },
            { "password", p.password },
            { "identity", p.identity },
            { "player_type", p.player_type }
        };
    }

    template<template<typename...> class Traits>
    static void to(const tao::json::basic_value<Traits>& v, database::collection::PlayerLogin& p)
    {
        v.at("username").to(p.username);
        v.at("password").to(p.password);
        v.at("identity").to(p.identity);
        v.at("player_type").to(p.player_type);
    }
};

template<>
struct tao::json::traits<database::collection::PlayerLoginSession> {
    template<template<typename...> class Traits>
    static void assign(tao::json::basic_value<Traits>& v, const database::collection::PlayerLoginSession& p)
    {
        v = {
            { "connection_key", p.connection_key }
        };
    }

    template<template<typename...> class Traits>
    static void to(const tao::json::basic_value<Traits>& v, database::collection::PlayerLoginSession& p)
    {
        v.at("connection_key").to(p.connection_key);
    }
};