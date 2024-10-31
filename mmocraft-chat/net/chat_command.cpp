#include "chat_command.h"

namespace chat
{
namespace net
{
    void ChatCommand::execute(std::string_view sender_player_name, std::string_view command)
    {
        util::string_copy(_sender_player_name, sender_player_name);
        ::memcpy_s(_command, sizeof(_command), command.data(), command.size());

        try {
            auto tokens = parse_tokens(_command);
            if (tokens.size() == 0)
                throw std::invalid_argument("");

            auto program = tokens[0];

            if (!std::strcmp("/dm", program))
                execute_direct_message(tokens);
            else
                set_error("Unsupported command");
        }
        catch (std::invalid_argument const&) {
            set_error("Invalid command");
        }
    }

    void ChatCommand::execute_direct_message(const std::vector<const char*>& tokens)
    {
        if (tokens.size() != 3) {
            set_error("Usage: /dm TO MESSAGE");
            return;
        }

        auto receiver_player_name = tokens[1];
        auto message = tokens[2];

        format_from_message(_sender_response, _sender_player_name, message);
        format_to_message(_receiver_response, receiver_player_name, message);
    }

    std::vector<const char*> ChatCommand::parse_tokens(char* command)
    {
        std::vector<const char*> tokens;

        bool token_splited = true;
        for (int i = 0; i < sizeof(_command); i++) {
            if (util::is_whitespace(command[i])) {
                command[i] = '\0';
                token_splited = true;
            }
            else if (token_splited) {
                tokens.push_back(command + i);
                token_splited = false;
            }
        }

        return tokens;
    }

    void ChatCommand::set_response(const char* res)
    {
        ::strcpy_s(_receiver_response, res);
    }

    void ChatCommand::set_error(const char* res)
    {
        ::strcpy_s(_sender_response, ERROR_COLOR);
        ::strcat_s(_sender_response, res);
    }
}
}