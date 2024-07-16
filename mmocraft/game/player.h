#pragma once

#include <bitset>
#include <string.h>

#include "net/packet_id.h"
#include "net/connection_key.h"
#include "util/common_util.h"
#include "util/math.h"

#include "proto/player_gamedata.pb.h"

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

        inline PlayerPosition(std::uint64_t a_raw = 0)
            : raw{a_raw}
        { }

        inline std::uint64_t raw_coordinate() const
        {
            return raw & coordinate_mask;
        }

        inline static std::uint64_t raw_coordinate(std::uint64_t raw)
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

        inline static std::uint64_t raw_orientation(std::uint64_t raw)
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

        const char* username() const
        {
            return _username;
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
            return _gamedata.spawn_pos();
        }

        auto spawn_yaw() const
        {
            PlayerPosition pos(_gamedata.spawn_pos());
            return pos.view.yaw;
        }

        auto spawn_pitch() const
        {
            PlayerPosition pos(_gamedata.spawn_pos());
            return pos.view.pitch;
        }

        void set_position(PlayerPosition pos)
        {
            _gamedata.set_latest_pos(pos.raw);
        }

        void set_spawn_position(PlayerPosition pos)
        {
            _gamedata.set_spawn_pos(pos.raw);
        }

        void set_spawn_coordinate(int x, int y, int z, bool force = true)
        {
            PlayerPosition pos(_gamedata.spawn_pos());
            if (force || not pos.raw_coordinate()) {
                pos.set_raw_coordinate(x * 32, y * 32 + 51, z * 32);
                _gamedata.set_spawn_pos(pos.raw);
            }
        }

        void set_spawn_orientation(unsigned yaw, unsigned pitch, bool force = true)
        {
            PlayerPosition pos(_gamedata.spawn_pos());
            if (force || not pos.raw_orientation()) {
                pos.set_raw_orientation(yaw, pitch);
                _gamedata.set_spawn_pos(pos.raw);
            }
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
            return _gamedata.latest_pos();
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

        void set_extension_count(unsigned count)
        {
            pending_extension_count = count;
        }

        auto decrease_pending_extension_count()
        {
            return --pending_extension_count;
        }

        void register_extension(net::PacketID ext)
        {
            supported_extensions.set(ext);
        }

        bool is_supported_extension(net::PacketID ext)
        {
            return supported_extensions.test(ext);
        }

        void load_gamedata(const std::byte* data, std::size_t data_size)
        {
            _gamedata.ParseFromArray(data, int(data_size));
        }

        void copy_gamedata(std::byte* data, std::size_t data_size) const
        {
            assert(_gamedata.ByteSizeLong() <= data_size);
            _gamedata.SerializePartialToArray(data, int(data_size));
        }

    private:
        PlayerState _state = PlayerState::Initialized;

        net::ConnectionKey _connection_key;
        
        unsigned pending_extension_count = 0;
        std::bitset<64> supported_extensions;

        // Row number of the player table (so always greater than 0)
        // it's used to verify that no same (identity) users already logged in.
        unsigned _identity;

        PlayerType _player_type;

        char _username[16 + 1];
        game::PlayerGamedata _gamedata;

        PlayerPosition _last_transferred_pos;
        PlayerPosition _last_transferred_pos_tmp;

        std::size_t _last_ping_time = 0;
    };
}