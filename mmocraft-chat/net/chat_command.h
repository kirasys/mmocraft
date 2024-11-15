#pragma once

#include <string_view>
#include <vector>
#include <cstddef>
#include <cstdio>

#include <net/packet.h>

#define ERROR_COLOR "&c"
#define SUCCESS_COLOR "&2"
#define BLUE_COLOR "&9"

namespace chat
{
namespace net
{
    class ChatCommand
    {
    public:
        ChatCommand() = default;

        void execute(std::string_view sender_player_name, std::string_view command);

        void set_response(const char*);

        void set_error(const char*);

        const char* sender_name() const
        {
            return _sender_player_name;
        }

        const char* receiver_name() const
        {
            return _receiver_player_name;
        }

        const char* sender_response() const
        {
            return _sender_response;
        }

        const char* receiver_response() const
        {
            return _receiver_response;
        }

        bool has_sender_message() const
        {
            return sender_name()[0] || sender_response()[0];
        }

        bool has_receiver_message() const
        {
            return receiver_name()[0] || receiver_response()[0];
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

        //void execute_mail(const std::vector<const char*>&);

        //void execute_mail_read(const std::vector<const char*>&);

        //void execute_mail_write(unsigned player_id, const std::vector<const char*>&);

        //void execute_mail_delete(const std::vector<const char*>&);

        void execute_direct_message(const std::vector<const char*>&);

        //void execute_announcement(const std::vector<const char*>&);

        char _sender_player_name[16 + 1] = {};

        char _receiver_player_name[16 + 1] = {};

        char _command[::net::PacketFieldConstraint::max_string_length] = {};

        char _sender_response[::net::PacketFieldConstraint::max_string_length] = {};

        char _receiver_response[::net::PacketFieldConstraint::max_string_length] = {};
    };
}
}