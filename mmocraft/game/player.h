#pragma once

#include <string.h>

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

    enum PlayerState
    {
        Disconnected,

        Initialized,

        Handshake_Completed,

        Level_Initialized,

        Spawn_Wait,
    };

    struct Coordinate3D
    {
        short x = 0;
        short y = 0;
        short z = 0;
    };

    class Player : util::NonCopyable
    {
    public:
        Player(net::ConnectionKey, unsigned identity, PlayerType, const char* username, const char* password);

        PlayerState state() const
        {
            return _state;
        }

        void set_state(PlayerState state)
        {
            _state = state;
        }

        bool transit_state(PlayerState old_state, PlayerState new_state)
        {
            _state = _state == old_state ? new_state : old_state;
            return _state == new_state;
        }

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

        const char* player_name() const
        {
            return _username;
        }

        Coordinate3D spawn_position() const
        {
            return { short(_spawn_pos.x << 5), short(_spawn_pos.y << 5), short(_spawn_pos.z << 5) };
        }

        auto spawn_yaw() const
        {
            return _spawn_yaw;
        }

        auto spawn_pitch() const
        {
            return _spawn_pitch;
        }

        void set_default_spawn_position(int x, int y, int z)
        {
            if (_spawn_pos.x || _spawn_pos.y || _spawn_pos.z == 0)
                _spawn_pos = { short(x), short(y), short(z) };
        }

        void set_default_spawn_orientation(unsigned yaw, unsigned pitch)
        {
            if (_spawn_yaw || _spawn_pitch == 0) {
                _spawn_yaw = std::uint8_t(yaw);
                _spawn_pitch = std::uint8_t(pitch);
            }
        }

    private:
        PlayerState _state = PlayerState::Initialized;

        net::ConnectionKey _connection_key;

        // it's used to verify that no same (identity) users already logged in.
        // same as player row number of the database's table (so always greater than 0)
        unsigned _identity;

        PlayerType _player_type;

        char _username[16 + 1];
        char _password[32 + 1];

        Coordinate3D _spawn_pos = { 0, 0, 0 };
        std::uint8_t _spawn_yaw = 0;
        std::uint8_t _spawn_pitch = 0;
    };
}