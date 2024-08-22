#include "pch.h"
#include "player.h"

namespace game
{
    Player::Player(net::ConnectionKey a_connection_key, std::string_view username)
        : _connection_key{ a_connection_key }
    {
        util::string_copy(_username, username);
    }
}