#pragma once

#include <bitset>
#include <string.h>

#include "database/couchbase_definitions.h"

#include "net/packet_id.h"
#include "net/connection_key.h"
#include "util/common_util.h"
#include "util/math.h"

#include "proto/generated/player_gamedata.pb.h"

namespace game
{
    using PlayerID = std::uint8_t;

    // Warning: PlayerLogin procedure uses these enum as raw value.
    //          we must reflect modification to the procedure.
    enum class player_type_id
    {
        invalid,

        // Users logged in without password.
        // destory all information after disconnecting.
        guest,

        // Users logged in successfully.
        authenticated_user,

        // Users with administrator privileges.
        admin,
    };

    class PlayerState
    {
    public:
        enum State
        {
            // Error states
            disconnecting,

            disconnected,

            // Sequential states
            initialized,

            handshaking,

            ex_handshaking,

            handshaked,

            ex_handshaked,

            extension_syncing,

            extension_synced,

            level_initializing,

            level_initialized,

            spawning,

            spawned,
        };

        auto state() const
        {
            return _cur;
        }

        auto prev_state() const
        {
            return _prev;
        }

        auto next_state() const
        {
            return _next;
        }

        void transit_state()
        {
            _prev = _cur;
            _cur = _next;
        }

        void prepare_state_transition(State cur, State next)
        {
            _prev = _cur;
            _cur = cur;
            _next = next;
        }

    private:
        State _prev = State::initialized;
        State _cur = State::initialized;
        State _next = State::initialized;
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

    class Player : public PlayerState, util::NonCopyable
    {
    public:
        Player(net::ConnectionKey, std::string_view username);

        net::ConnectionKey connection_key() const
        {
            return _connection_key;
        }

        const std::string& uuid() const
        {
            return _uuid;
        }

        void set_uuid(const std::string& uuid)
        {
            _uuid = uuid;
        }

        game::player_type_id player_type() const
        {
            return _player_type;
        }

        void set_player_type(player_type_id type)
        {
            _player_type = type;
        }

        const char* username() const
        {
            return _username;
        }

        PlayerID game_id() const
        {
            return PlayerID(_connection_key.index());
        }

        PlayerPosition spawn_position() const
        {
            return _gamedata.spawn_pos;
        }

        auto spawn_yaw() const
        {
            PlayerPosition pos(spawn_position());
            return pos.view.yaw;
        }

        auto spawn_pitch() const
        {
            PlayerPosition pos(spawn_position());
            return pos.view.pitch;
        }

        void set_position(PlayerPosition pos)
        {
            _gamedata.latest_pos = pos.raw;
        }

        void set_spawn_position(PlayerPosition pos)
        {
            _gamedata.spawn_pos = pos.raw;
        }

        void set_spawn_coordinate(int x, int y, int z, bool force = true)
        {
            PlayerPosition spawn_pos(spawn_position());
            if (force || spawn_pos.raw_coordinate() == 0) {
                spawn_pos.set_raw_coordinate(x * 32, y * 32 + 51, z * 32);
                _gamedata.spawn_pos = spawn_pos.raw;
            }
        }

        void set_spawn_orientation(unsigned yaw, unsigned pitch, bool force = true)
        {
            PlayerPosition spawn_pos(spawn_position());
            if (force || spawn_pos.raw_orientation() == 0) {
                spawn_pos.set_raw_orientation(yaw, pitch);
                _gamedata.spawn_pos = spawn_pos.raw;
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
            return _gamedata.latest_pos;
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

        void set_extension_mode()
        {
            extension_mode = true;
        }

        void set_extension_count(unsigned count)
        {
            pending_extension_count = count;
        }

        auto decrease_pending_extension_count()
        {
            return --pending_extension_count;
        }

        void register_extension(net::packet_type_id::value ext)
        {
            supported_extensions.set(ext);
        }

        bool is_support_extension() const
        {
            return extension_mode;
        }

        bool is_supported_extension(net::packet_type_id::value ext) const
        {
            return supported_extensions.test(ext);
        }

        void set_gamedata(const database::collection::PlayerGamedata& gamedata)
        {
            _gamedata = gamedata;
        }

        const database::collection::PlayerGamedata& get_gamedata() const
        {
            return _gamedata;
        }

    private:

        net::ConnectionKey _connection_key;

        game::player_type_id _player_type;

        char _username[16 + 1];

        database::collection::PlayerGamedata _gamedata;

        std::string _uuid;

        bool extension_mode = false;
        unsigned pending_extension_count = 0;
        std::bitset<64> supported_extensions;

        PlayerPosition _last_transferred_pos;
        PlayerPosition _last_transferred_pos_tmp;

        std::size_t _last_ping_time = 0;
    };
}