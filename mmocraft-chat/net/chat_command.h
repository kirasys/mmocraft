#pragma once

#include <string_view>
#include <vector>
#include <cstddef>
#include <cstdio>

namespace chat
{
namespace net
{
    class ChatCommand
    {
    public:
        ChatCommand(std::string_view sender_name)
            : _player{ *player }
        { }

        void execute(std::string_view);

        void set_response(const char*);

        const char* get_response() const
        {
            return _response;
        }

    private:
        std::vector<const char*> parse_tokens(char*);

        template <std::size_t N>
        int format_from_message(char(&buffer)[N], const char* from, const char* message)
        {
            return std::max(std::snprintf(buffer, N, BLUE_COLOR "[from %s] %s", from, message), 0);
        }

        template <std::size_t N>
        int format_to_message(char(&buffer)[N], const char* to, const char* message)
        {
            return std::max(std::snprintf(buffer, N, BLUE_COLOR "[to %s] %s", to, message), 0);
        }

        void execute_mail(const std::vector<const char*>&);

        void execute_mail_read(const std::vector<const char*>&);

        void execute_mail_write(unsigned player_id, const std::vector<const char*>&);

        void execute_mail_delete(const std::vector<const char*>&);

        void execute_direct_message(const std::vector<const char*>&);

        void execute_announcement(const std::vector<const char*>&);

        char _command[128] = {};

        char _response[128] = {};
    };
}
}