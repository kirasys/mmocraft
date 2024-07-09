#pragma once

#include <vector>
#include <string_view>

#include "game/player.h"

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
        std::vector<std::string_view> get_lexical_tokens(std::string_view);

        void execute_set_spawn(const std::vector<std::string_view>&);

        void execute_announcement(game::World&, const std::vector<std::string_view>&);

        game::Player& _player;
        char _response[64] = { 0 };
    };
}