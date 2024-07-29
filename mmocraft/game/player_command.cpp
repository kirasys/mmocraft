#include "pch.h"
#include "player_command.h"

#include <string.h>
#include <stdexcept>

#include "database/query.h"
#include "game/world.h"
#include "logging/logger.h"

namespace game
{
    void PlayerCommand::execute(game::World& world, std::string_view command)
    {
        ::memcpy_s(_command, sizeof(_command), command.data(), command.size());

        try {
            auto tokens = parse_tokens(_command);
            if (tokens.size() == 0)
                throw std::invalid_argument("");

            auto program = tokens[0];

            if (!std::strcmp("/set_spawn", program))
                execute_set_spawn(tokens);
            else if (!std::strcmp("/mail", program))
                execute_mail(tokens);
            else if (!std::strcmp("/dm", program))
                execute_direct_message(world, tokens);
            else if (!std::strcmp("/announcement", program))
                execute_announcement(world, tokens);
            else
                set_response(ERROR_COLOR "Unsupported command");
        }
        catch (std::invalid_argument const&) {
            set_response(ERROR_COLOR "Invalid command");
        }
    }

    void PlayerCommand::execute_set_spawn(const std::vector<const char*>& tokens)
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

    void PlayerCommand::execute_mail(const std::vector<const char*>& tokens)
    {
        if (tokens.size() < 3) {
            set_response(ERROR_COLOR "Usage: /mail MODE(read | write | delete) USERNAME ...");
            return;
        }

        auto username = tokens[1];
        auto mode = tokens[2];
        
        database::PlayerSearchSQL player_search;
        player_search.search(username);

        if (!std::strcmp("write", mode))
            execute_mail_write(player_search.player_identity(), tokens);
    }

    void PlayerCommand::execute_mail_write(unsigned player_id, const std::vector<const char*>& tokens)
    {
        if (tokens.size() != 4) {
            set_response(ERROR_COLOR "Usage: /mail write USERNAME MESSAGE");
            return;
        }

        char message[net::PacketFieldConstraint::max_string_length];
        format_from_message(message, _player.username(), tokens[3]);
    }

    void PlayerCommand::execute_direct_message(game::World& world, const std::vector<const char*>& tokens)
    {
        if (tokens.size() != 3) {
            set_response(ERROR_COLOR "Usage: /dm TO MESSAGE");
            return;
        }

        char from_message[net::PacketFieldConstraint::max_string_length];
        format_from_message(from_message, _player.username(), tokens[2]);

        char to_message[net::PacketFieldConstraint::max_string_length];
        format_to_message(to_message, tokens[1], tokens[2]);

        world.unicast_to_world_player(_player.username(), net::MessageType::Chat, to_message);
        world.unicast_to_world_player(tokens[1], net::MessageType::Chat, from_message);
    }

    void PlayerCommand::execute_announcement(game::World& world, const std::vector<const char*>& tokens)
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

    std::vector<const char*> PlayerCommand::parse_tokens(char* command)
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

    void PlayerCommand::set_response(const char* res)
    {
        ::strcpy_s(_response, res);
    }
}