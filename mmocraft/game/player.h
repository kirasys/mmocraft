#pragma once

#include <string.h>

#include "net/packet.h"
#include "net/connection_key.h"
#include "util/common_util.h"

namespace game
{
    using PlayerID = unsigned;

    enum PlayerType
    {
        INVALID,

        // Users logged in without password.
        // destory all information after disconnecting.
        GUEST,

        // Users logged in with password but is not registered. 
        // they can register by entering command.
        NEW_USER,

        // Users logged in successfully.
        AUTHENTICATED_USER,

        // Users with administrator privileges.
        ADMIN,
    };

    class Player : util::NonCopyable
    {
    public:
        Player(net::ConnectionKey, unsigned identity, PlayerType, const char* username, const char* password);

        net::ConnectionKey connection_key() const
        {
            return _connection_key;
        }

        unsigned identity() const
        {
            return _identity;
        }

        PlayerType player_type() const
        {
            return _player_type;
        }

        PlayerID game_id() const
        {
            return PlayerID(_connection_key.index());
        }


    private:
        net::ConnectionKey _connection_key;

        // it's used to verify that no same (identity) users already logged in.
        // same as player row number of the database's table (so always greater than 0)
        unsigned _identity;

        PlayerType _player_type;

        char _username[net::PacketFieldConstraint::max_username_length + 1];
        char _password[net::PacketFieldConstraint::max_password_length + 1];
    };
}