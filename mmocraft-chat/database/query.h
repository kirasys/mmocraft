#pragma once

#include <string_view>

namespace chat_database
{
    void connect_mongodb_server(std::string_view uri);

    class MailDocument
    {
    public:
        static constexpr const char* collection_name = "mail";

        MailDocument() = default;

        void insert(const char*);
    };
}