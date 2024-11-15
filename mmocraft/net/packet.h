#pragma once

#include <utility>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <string_view>

#include "net/packet_id.h"
#include "io/io_event.h"
#include "logging/error.h"
#include "game/player.h"
#include "game/block.h"
#include "util/compressor.h"
#include "util/math.h"

namespace net
{
    namespace PacketFieldType
    {
        using Byte = std::uint8_t;
        using SByte = std::int8_t;
        using FByte = std::int8_t;
        using Short = std::int16_t;
        using FShort = std::uint16_t;
        using Int = std::int32_t;
        using UInt64 = std::uint64_t;

        using String = std::string_view;
    };

    namespace PacketFieldConstraint
    {
        static constexpr std::size_t max_username_length = 16;
        static constexpr std::size_t max_password_length = 32;
        static constexpr std::size_t max_string_length = 64;
    }

    struct Packet
    {
        Packet() : id{ PacketFieldType::Byte(net::packet_type_id::invalid) } { }
        Packet(net::packet_type_id::value a_id) : id{ PacketFieldType::Byte(a_id) } { }
        virtual ~Packet() = default;

        PacketFieldType::Byte id;
    };

    struct PacketHandshake : Packet
    {
        PacketFieldType::Byte protocol_version;
        union {
            PacketFieldType::String username;
            PacketFieldType::String server_name;
        };
        union {
            PacketFieldType::String password;
            PacketFieldType::String motd;
        };
        union {
            PacketFieldType::Byte cpe_magic;
            PacketFieldType::Byte user_type;
        };

        static constexpr net::packet_type_id::value packet_id = net::packet_type_id::handshake;
        static constexpr std::size_t packet_size = 131;

        PacketHandshake(const std::byte* data)
            : Packet{ packet_id }
        { 
            parse(data);
        }

        PacketHandshake(std::string_view a_server_name, std::string_view a_motd, int a_user_type)
            : Packet{ packet_id }
            , server_name{ a_server_name.data(), a_server_name.size() }
            , motd{ a_motd.data(), a_motd.size() }
            , protocol_version{ 7 }
            , user_type{ PacketFieldType::Byte(a_user_type) }
        { }

        void parse(const std::byte* buf_start);

        error::ErrorCode validate();

        bool serialize(io::IoEventData&) const;

        bool has_cpe_support() const
        {
            return cpe_magic == 0x42;
        }
    };

    struct PacketPing : Packet
    {
        static constexpr net::packet_type_id::value packet_id = net::packet_type_id::ping;
        static constexpr std::size_t packet_size = 1;

        PacketPing(const std::byte* data)
            : Packet{ packet_id }
        { 
            parse(data);
        }

        void parse(const std::byte* buf_start);
    };

    struct PacketLevelInit : Packet
    {
        static constexpr net::packet_type_id::value packet_id = net::packet_type_id::level_initialize;
        static constexpr std::size_t packet_size = 1;

        PacketLevelInit()
            : Packet{ packet_id }
        { }

        bool serialize(io::IoEventData&) const;
    };

    struct PacketLevelDataChunk : Packet
    {
        static constexpr unsigned chunk_size = 1024;
        static constexpr std::size_t packet_size = 1028;

        std::size_t max_chunk_count = 0;
        PacketFieldType::Short x;
        PacketFieldType::Short y;
        PacketFieldType::Short z;

        PacketLevelDataChunk(std::byte* block_data, unsigned block_data_size, PacketFieldType::Short, PacketFieldType::Short, PacketFieldType::Short);

        std::size_t serialize(std::unique_ptr<std::byte[]>&);
        
    private:
        util::Compressor compressor;
    };

    struct PacketSetBlockClient : Packet
    {
        PacketFieldType::Short x;
        PacketFieldType::Short y;
        PacketFieldType::Short z;
        PacketFieldType::Byte mode;
        PacketFieldType::Byte block_id;

        static constexpr net::packet_type_id::value packet_id = net::packet_type_id::set_block_client;
        static constexpr std::size_t packet_size = 9;

        PacketSetBlockClient(const std::byte* data)
            : Packet{ packet_id }
        { 
            parse(data);
        }

        PacketSetBlockClient(util::Coordinate3D pos, PacketFieldType::Byte a_mode, PacketFieldType::Byte a_block_id)
            : Packet{ packet_id }
            , x{ pos.x }, y{ pos.y }, z{ pos.z }
            , mode{ a_mode }
            , block_id{ a_block_id }
        { }

        void parse(const std::byte* buf_start);

        bool serialize(io::IoEventData&) const;

        auto block_creation_mode() const
        {
            return game::block_creation_mode(mode);
        }
    };

    struct PacketSetBlockServer : Packet
    {
        util::Coordinate3D block_pos;
        PacketFieldType::Byte block_id;

        static constexpr net::packet_type_id::value packet_id = net::packet_type_id::set_block_server;
        static constexpr std::size_t packet_size = 8;

        PacketSetBlockServer(util::Coordinate3D pos, PacketFieldType::Byte a_block_id)
            : Packet{ packet_id }
            , block_pos{ pos }
            , block_id{ a_block_id }
        { }

        bool serialize(io::IoEventData&) const;
    };

    struct PacketSetPlayerPosition : Packet
    {
        PacketFieldType::Byte player_id;
        game::PlayerPosition  player_pos;

        static constexpr net::packet_type_id::value packet_id = net::packet_type_id::set_player_position;
        static constexpr std::size_t packet_size = 10;

        PacketSetPlayerPosition(const std::byte* data)
            : Packet{ packet_id }
        { 
            parse(data);
        }

        void parse(const std::byte* buf_start);

        static std::size_t serialize(const std::vector<game::Player*>&, std::unique_ptr<std::byte[]>&);
    };

    struct PacketSpawnPlayer : Packet
    {
        static constexpr net::packet_type_id::value packet_id = net::packet_type_id::spawn_player;
        static constexpr std::size_t packet_size = 74;

        PacketSpawnPlayer()
            : Packet{ packet_id }
        { }

        static std::size_t serialize(const std::vector<game::Player*>&, const std::vector<game::Player*>&, std::unique_ptr<std::byte[]>&);

    };

    struct PacketDespawnPlayer : Packet
    {
        static constexpr net::packet_type_id::value packet_id = net::packet_type_id::despawn_player;
        static constexpr std::size_t packet_size = 2;

        static std::size_t serialize(const std::vector<game::Player*>&, std::unique_ptr<std::byte[]>&);
    };
    
    struct PacketChatMessage : Packet
    {
        PacketFieldType::Byte player_id;
        PacketFieldType::String message;

        static constexpr net::packet_type_id::value packet_id = net::packet_type_id::chat_message;
        static constexpr std::size_t packet_size = 66;

        PacketChatMessage(const std::byte* data)
            : Packet{ packet_id }
        {
            parse(data);
        }

        PacketChatMessage(std::string_view msg)
            : Packet { packet_id }
            , player_id{ 0xFF }
            , message{ msg.data(), msg.size() }
        { }

        void parse(const std::byte* buf_start);

        bool serialize(io::IoEventData&) const;

        bool is_commmand_message() const
        {
            return not message.empty() && message[0] == '/';
        }
    };

    struct PacketDisconnectPlayer : Packet
    {
        PacketFieldType::String reason;

        static constexpr net::packet_type_id::value packet_id = net::packet_type_id::disconnect_player;
        constexpr static std::size_t packet_size = 65;

        PacketDisconnectPlayer(std::string_view a_reason)
            : Packet{ packet_id }
            , reason{ a_reason.data(), a_reason.size() }
        { }

        bool serialize(io::IoEventData&) const;
    };

    struct PacketSetPlayerID : Packet
    {
        PacketFieldType::Byte player_id;

        static constexpr net::packet_type_id::value packet_id = net::packet_type_id::set_playerid;
        constexpr static std::size_t packet_size = 2;

        PacketSetPlayerID(unsigned a_player_id)
            : Packet{ packet_id }
            , player_id(PacketFieldType::Byte(a_player_id))
        { }

        bool serialize(io::IoEventData&) const;
    };

    struct PacketStructure
    {
        consteval static std::size_t max_size_of_packet_struct()
        {
            return sizeof(PacketHandshake);
        }

        static auto parse_packet(std::byte* buf_start, std::byte* buf_end, Packet*) -> std::pair<std::uint32_t, error::ResultCode>;

        static std::pair<net::packet_type_id::value, int> parse_packet(const std::byte* buf_start);

        static inline void read_scalar(const std::byte*& buf, PacketFieldType::Byte &value)
        {
            value = *reinterpret_cast<const PacketFieldType::Byte*>(buf++);
        }

        static inline void read_scalar(const std::byte*& buf, PacketFieldType::Short& value)
        {
            value = _byteswap_ushort(*reinterpret_cast<const PacketFieldType::Short*>(buf));
            buf += sizeof(PacketFieldType::Short);
        }

        static inline void read_scalar(const std::byte*& buf, PacketFieldType::Int& value)
        {
            value = _byteswap_ulong(*reinterpret_cast<const PacketFieldType::Int*>(buf));
            buf += sizeof(PacketFieldType::Int);
        }

        static inline void read_scalar(const std::byte*& buf, PacketFieldType::UInt64& value)
        {
            value = _byteswap_uint64(*reinterpret_cast<const PacketFieldType::UInt64*>(buf));
            buf += sizeof(PacketFieldType::UInt64);
        }

        static void read_string(const std::byte*& buf, PacketFieldType::String& value);

        static inline void write_byte(std::byte*& buf, PacketFieldType::Byte value)
        {
            *buf++ = std::byte(value);
        }

        static inline void write_short(std::byte*& buf, PacketFieldType::Short value)
        {
            *reinterpret_cast<decltype(value)*>(buf) = _byteswap_ushort(value);
            buf += sizeof(value);
        }

        static inline void write_int(std::byte*& buf, PacketFieldType::Int value)
        {
            *reinterpret_cast<decltype(value)*>(buf) = _byteswap_ulong(value);
            buf += sizeof(value);
        }

        static inline void write_uint64(std::byte*& buf, std::uint64_t value)
        {
            *reinterpret_cast<decltype(value)*>(buf) = _byteswap_uint64(value);
            buf += sizeof(value);
        }

        static void write_string(std::byte*&, const PacketFieldType::String&);

        static void write_string(std::byte*&, const char*);

        static void write_string(std::byte*& buf, const std::byte* data, std::size_t data_size);

        static void write_coordinate(std::byte*&, util::Coordinate3D);

        static void write_orientation(std::byte*&, std::uint8_t yaw, std::uint8_t pitch);
    };
}