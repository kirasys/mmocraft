#pragma once

#include <cstdlib>
#include <cstring>
#include <vector>
#include <string_view>

#include "game/player.h"

#define ERROR_COLOR "&c"
#define SUCCESS_COLOR "&2"
#define BLUE_COLOR "&9"

namespace game
{
    class World;

    class PlayerCommand
    {
    public:
        PlayerCommand(game::Player* player)
            : _player{ *player }
        { }

        void execute(game::World&, std::string_view);

        void set_response(const char*);

        const char* get_response() const
        {
            return _response;
        }

    private:
        std::vector<const char*> parse_tokens(char*);

        void execute_set_spawn(const std::vector<const char*>&);

        template <std::size_t N>
        int format_from_message(char (&buffer)[N], const char* from, const char* message)
        {
            return std::max(std::snprintf(buffer, N, BLUE_COLOR "[from %s] %s", from, message), 0);
        }

        template <std::size_t N>
        int format_to_message(char (&buffer)[N], const char* to, const char* message)
        {
            return std::max(std::snprintf(buffer, N, BLUE_COLOR "[to %s] %s", to, message), 0);
        }

        void execute_mail(const std::vector<const char*>&);

        void execute_mail_read(const std::vector<const char*>&);

        void execute_mail_write(const std::vector<const char*>&);

        void execute_mail_delete(const std::vector<const char*>&);

        void execute_direct_message(game::World&, const std::vector<const char*>&);

        void execute_announcement(game::World&, const std::vector<const char*>&);

        game::Player& _player;

        char _command[64] = {};

        char _response[64] = {};
    };
}