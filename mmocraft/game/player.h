#pragma once

#include <string.h>

#include "net/connection_key.h"
#include "util/common_util.h"
#include "util/math.h"

namespace game
{
    using PlayerID = std::uint8_t;

    enum PlayerType
    {
        INVALID,

        // Users logged in without password.
        // destory all information after disconnecting.
        GUEST,

        // Users logged in successfully.
        AUTHENTICATED_USER,

        // Users with administrator privileges.
        ADMIN,
    };

    enum PlayerState
    {
        Disconnect_Completed,

        Disconnect_Wait,

        Initialized,

        Handshake_Completed,

        Level_Wait,

        Level_Initialized,

        Spawn_Wait,

        Spawned,
    };

    union PlayerPosition {
        std::uint64_t raw = 0;
        struct {
            std::int16_t x;
            std::int16_t y;
            std::int16_t z;
            std::uint8_t yaw;
            std::uint8_t pitch;
        } view;

        static constexpr std::uint64_t coordinate_mask  = 0x0000FFFFFFFFFFFF;
        static constexpr std::uint64_t orientation_mask = 0xFFFF000000000000;

        inline std::uint64_t raw_coordinate() const
        {
            return raw & coordinate_mask;
        }

        inline util::Coordinate3D coordinate() const
        {
            return { view.x, view.y, view.z };
        }

        inline std::uint64_t raw_orientation() const
        {
            return raw & orientation_mask;
        }

        inline std::uint8_t yaw() const
        {
            return view.yaw;
        }

        inline std::uint8_t pitch() const
        {
            return view.pitch;
        }

        inline void set_raw_coordinate(int x, int y, int z)
        {
            raw = std::uint64_t(x & 0xFFFF) | (std::uint64_t(y & 0xFFFF) << 16) | (std::uint64_t(z & 0xFFFF) << 32)
                | raw_orientation();
        }

        inline void set_raw_orientation(unsigned yaw, unsigned pitch)
        {
            raw = raw_coordinate() | (std::uint64_t(yaw & 0xFF) << 48) | (std::uint64_t(pitch & 0xFF) << 56);
        }

        PlayerPosition operator-(PlayerPosition rhs) const
        {
            PlayerPosition diff;
            diff.view.x = view.x - rhs.view.x;
            diff.view.y = view.y - rhs.view.y;
            diff.view.z = view.z - rhs.view.z;
            diff.view.yaw = view.yaw - rhs.view.yaw;
            diff.view.pitch = view.pitch - rhs.view.pitch;
            return diff;
        }

        PlayerPosition& operator=(const PlayerPosition& rhs)
        {
            raw = rhs.raw;
            return *this;
        }
    };

    class Player : util::NonCopyable
    {
    public:
        Player(net::ConnectionKey, unsigned identity, PlayerType, const char* username);

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

        PlayerPosition spawn_position() const
        {
            return _spawn_pos;
        }

        auto spawn_yaw() const
        {
            return _spawn_pos.view.yaw;
        }

        auto spawn_pitch() const
        {
            return _spawn_pos.view.pitch;
        }

        void set_position(PlayerPosition pos)
        {
            _latest_pos.raw = pos.raw;
        }

        void set_default_spawn_coordinate(int x, int y, int z)
        {
            if (not _spawn_pos.raw_coordinate())
                _spawn_pos.set_raw_coordinate(x * 32, y * 32 + 51, z * 32);
        }

        void set_default_spawn_orientation(unsigned yaw, unsigned pitch)
        {
            if (not _spawn_pos.raw_orientation())
                _spawn_pos.set_raw_orientation(yaw, pitch);
        }

        void update_last_transferred_position(PlayerPosition pos)
        {
            _last_transferred_pos_tmp = pos;
        }

        void commit_last_transferrd_position()
        {
            _last_transferred_pos = _last_transferred_pos_tmp;
        }

        PlayerPosition last_position() const
        {
            return _latest_pos;
        }

        PlayerPosition last_transferred_position() const
        {
            return _last_transferred_pos;
        }

        std::size_t last_ping_time() const
        {
            return _last_ping_time;
        }

        void update_ping_time()
        {
            _last_ping_time = util::current_monotonic_tick();
        }

    private:
        PlayerState _state = PlayerState::Initialized;

        net::ConnectionKey _connection_key;

        // it's used to verify that no same (identity) users already logged in.
        // same as player row number of the database's table (so always greater than 0)
        unsigned _identity;

        PlayerType _player_type;

        char _username[16 + 1];

        PlayerPosition _latest_pos;
        PlayerPosition _last_transferred_pos;
        PlayerPosition _last_transferred_pos_tmp;
        PlayerPosition _spawn_pos;

        std::size_t _last_ping_time = 0;
    };
}