#include "pch.h"
#include "player.h"

namespace game
{
    Player::Player(net::ConnectionKey a_connection_key, unsigned identity, PlayerType player_type, const char* username)
        : _connection_key{ a_connection_key }
        , _identity{ identity }
        , _player_type{ player_type }
    {
        ::strcpy_s(_username, username);
    }
}