#include "pch.h"
#include "player.h"

namespace game
{
    Player::Player(PlayerID player_id, PlayerType player_type, const char* username, const char* password)
        : _id{ player_id }
        , player_type{ player_type }
    {
        ::strcpy_s(_username, username);
        if (player_type == PlayerType::NEW_USER)
            ::strcpy_s(_password, password);
    }
}