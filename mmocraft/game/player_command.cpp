#include "pch.h"
#include "player_command.h"

#include <cstdlib>
#include <cstring>
#include <string.h>
#include <stdexcept>

#include "game/world.h"
#include "logging/logger.h"

#define ERROR_COLOR "&c"
#define SUCCESS_COLOR "&2"
#define BLUE_COLOR "&9"

namespace game
{
    void PlayerCommand::execute(game::World& world, std::string_view command)
    {
        try {
            auto tokens = get_lexical_tokens(command);
            if (tokens.size() == 0)
                throw std::invalid_argument("");

            auto program = tokens[0];
            if (program == "/set_spawn")
                execute_set_spawn(tokens);
            else if (program == "/dm")
                execute_direct_message(world, tokens);
            else if (program == "/announcement")
                execute_announcement(world, tokens);
            else
                set_response(ERROR_COLOR "Unsupported command");
        }
        catch (std::invalid_argument const&) {
            set_response(ERROR_COLOR "Invalid command");
        }
    }

    void PlayerCommand::execute_set_spawn(const std::vector<std::string_view>& tokens)
    {
        if (tokens.size() != 4) {
            set_response(ERROR_COLOR "Usage: /set_spawn X Y Z");
            return;
        }

        auto x = util::to_integer<int>(tokens[1]);
        auto y = util::to_integer<int>(tokens[2]);
        auto z = util::to_integer<int>(tokens[3]);
        _player.set_spawn_coordinate(x, y, z);

        set_response(SUCCESS_COLOR "Spawn point saved.");
    }

    void PlayerCommand::execute_direct_message(game::World& world, const std::vector<std::string_view>& tokens)
    {
        if (tokens.size() != 3) {
            set_response(ERROR_COLOR "Usage: /dm TO MESSAGE");
            return;
        }

        char from_message[64];
        std::snprintf(from_message, sizeof(from_message), BLUE_COLOR "[from %.*s] %.*s",
            static_cast<int>(tokens[1].size()), tokens[1].data(),
            static_cast<int>(tokens[2].size()), tokens[2].data());

        char to_message[64];
        std::snprintf(to_message, sizeof(to_message), BLUE_COLOR "[to %.*s] %.*s",
            static_cast<int>(tokens[1].size()), tokens[1].data(),
            static_cast<int>(tokens[2].size()), tokens[2].data());

        world.unicast_to_world_player(_player.username(), net::MessageType::Chat, to_message);
        world.unicast_to_world_player(tokens[1], net::MessageType::Chat, from_message);
    }

    void PlayerCommand::execute_announcement(game::World& world, const std::vector<std::string_view>& tokens)
    {
        if (tokens.size() != 2) {
            set_response(ERROR_COLOR "Usage: /announcement MESSAGE");
            return;
        }

        if (_player.player_type() != game::PlayerType::ADMIN) {
            set_response(ERROR_COLOR "Permission denied");
            return;
        }

        world.broadcast_to_world_player(net::MessageType::Announcement, tokens[1]);
    }

    std::vector<std::string_view> PlayerCommand::get_lexical_tokens(std::string_view command)
    {
        std::vector<std::string_view> tokens;

        int token_start = -1;
        for (int i = 0; i < command.size(); i++) {
            if (token_start < 0 && not util::is_whitespace(command[i]))
                token_start = i;
            else if (token_start >= 0 && util::is_whitespace(command[i])) {
                tokens.push_back(command.substr(token_start, i - token_start));
                token_start = -1;
            }
        }

        if (token_start >= 0)
            tokens.push_back(command.substr(token_start, command.size() - token_start));

        return tokens;
    }

    void PlayerCommand::set_response(const char* res)
    {
        ::strcpy_s(_response, res);
    }
}