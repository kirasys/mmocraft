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
    
    enum CollectionPath
    {
        // standard_bucket._default
        player_login,
        player_gamedata,

        chat_message_common,
        chat_message_private,
        chat_message_permanent,

        // cached_bucket._default
        player_login_session,

        SIZE,
    };

    constexpr std::string to_string(CollectionPath path)
    {
        // Warning: associated string should never be changed without a migration.
        constexpr const char* collection_path_strs[] = {
            // standard_bucket._default
            "player_login",
            "player_gamedata",

            "chat_message_common",
            "chat_message_private",
            "chat_message_permanent",

            // cached_bucket._default
            "player_login_session"
        };

        static_assert(CollectionPath::SIZE == std::size(collection_path_strs));

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

        struct PlayerGamedata
        {
            std::uint64_t latest_pos = 0;
            std::uint64_t spawn_pos = 0;
            std::uint32_t level = 0;
            std::uint32_t exp = 0;
        };

        struct ChatMessage
        {
            std::string sender_name;
            std::string receiver_name;
            std::string message;
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

template<>
struct tao::json::traits<database::collection::PlayerGamedata> {
    template<template<typename...> class Traits>
    static void assign(tao::json::basic_value<Traits>& v, const database::collection::PlayerGamedata& p)
    {
        v = {
            { "latest_pos", p.latest_pos },
            { "spawn_pos", p.spawn_pos },
            { "level", p.level },
            { "exp", p.exp }
        };
    }

    template<template<typename...> class Traits>
    static void to(const tao::json::basic_value<Traits>& v, database::collection::PlayerGamedata& p)
    {
        v.at("latest_pos").to(p.latest_pos);
        v.at("spawn_pos").to(p.spawn_pos);
        v.at("level").to(p.level);
        v.at("exp").to(p.exp);
    }
};

template<>
struct tao::json::traits<database::collection::ChatMessage> {
    template<template<typename...> class Traits>
    static void assign(tao::json::basic_value<Traits>& v, const database::collection::ChatMessage& p)
    {
        v = {
            { "sender_name", p.sender_name },
            { "receiver_name", p.receiver_name },
            { "message", p.message }
        };
    }

    template<template<typename...> class Traits>
    static void to(const tao::json::basic_value<Traits>& v, database::collection::ChatMessage& p)
    {
        v.at("sender_name").to(p.sender_name);
        v.at("receiver_name").to(p.receiver_name);
        v.at("message").to(p.message);
    }
};