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
#include "util/compressor.h"
#include "util/math.h"

#define DECLARE_PACKET_READ_METHOD(packet_type) \
    static void parse(std::byte* buf_start, std::byte* buf_end, net::Packet*); \
                                                             \
    static error::ErrorCode validate(const net::Packet*);	 \
                                                             \
    static inline auto to_derived(net::Packet* packet)		 \
    {														 \
        return static_cast<packet_type*>(packet);			 \
    }														 \
    static inline auto to_derived(const net::Packet* packet) \
    {														 \
        return static_cast<const packet_type*>(packet);		 \
    }

namespace net
{
    enum UserType
    {
        NORMAL = 0,
        OP = 64,
    };

    namespace PacketFieldType
    {
        using Byte = std::uint8_t;
        using SByte = std::int8_t;
        using FByte = std::int8_t;
        using Short = std::int16_t;
        using FShort = std::uint16_t;
        using Int = std::int32_t;

        struct String
        {
            const char *data = nullptr;
            std::size_t size = 0;
            static constexpr std::size_t size_with_padding = 64;
        };
    };

    namespace PacketFieldConstraint
    {
        static constexpr std::size_t max_username_length = 16;
        static constexpr std::size_t max_password_length = 32;
        static constexpr std::size_t max_string_length = 64;
    }

    struct Packet
    {
        Packet(PacketID a_id) : id{ PacketFieldType::Byte(a_id) } { }
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

        static constexpr PacketID packet_id = PacketID::Handshake;
        constexpr static std::size_t packet_size = 131;

        DECLARE_PACKET_READ_METHOD(PacketHandshake);

        PacketHandshake(std::string_view a_server_name, std::string_view a_motd, net::UserType a_user_type)
            : Packet{ packet_id }
            , server_name{ a_server_name.data(), a_server_name.size() }
            , motd{ a_motd.data(), a_motd.size() }
            , protocol_version{ 7 }
            , user_type{ PacketFieldType::Byte(a_user_type) }
        { }

        bool serialize(io::IoEventData&) const;
    };

    struct PacketPing : Packet
    {
        constexpr static std::size_t packet_size = 1;

        DECLARE_PACKET_READ_METHOD(PacketHandshake);
    };

    struct PacketLevelInit : Packet
    {
        PacketLevelInit()
            : Packet{PacketID::LevelInitialize}
        { }

        constexpr static std::size_t packet_size = 1;

        bool serialize(io::IoEventData&) const;
    };

    struct PacketLevelDataChunk : Packet
    {
        constexpr static unsigned chunk_size = 1024;
        constexpr static std::size_t packet_size = 1028;

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

        static constexpr PacketID packet_id = PacketID::SetBlockClient;
        static constexpr std::size_t packet_size = 9;

        PacketSetBlockClient(util::Coordinate3D pos, PacketFieldType::Byte a_mode, PacketFieldType::Byte a_block_id)
            : Packet{ packet_id }
            , x{ pos.x }, y{ pos.y }, z{ pos.z }
            , mode{ a_mode }
            , block_id{ a_block_id }
        { }

        bool serialize(io::IoEventData&) const;

        DECLARE_PACKET_READ_METHOD(PacketSetBlockClient);
    };

    struct PacketSetBlockServer : Packet
    {
        util::Coordinate3D block_pos;
        PacketFieldType::Byte block_id;

        static constexpr PacketID packet_id = PacketID::SetBlockServer;
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

        static constexpr std::size_t packet_size = 10;

        DECLARE_PACKET_READ_METHOD(PacketSetPlayerPosition);

        static std::size_t serialize(const std::vector<game::Player*>&, std::unique_ptr<std::byte[]>&);
    };

    struct PacketSpawnPlayer : Packet
    {
        static constexpr PacketID packet_id = PacketID::SpawnPlayer;
        static constexpr std::size_t packet_size = 74;

        PacketSpawnPlayer()
            : Packet{ packet_id }
        { }

        static std::size_t serialize(const std::vector<game::Player*>&, const std::vector<game::Player*>&, std::unique_ptr<std::byte[]>&);

    };

    struct PacketDespawnPlayer : Packet
    {
        static constexpr PacketID packet_id = PacketID::DespawnPlayer;
        static constexpr std::size_t packet_size = 2;

        static std::size_t serialize(const std::vector<game::Player*>&, std::unique_ptr<std::byte[]>&);
    };
    
    struct PacketChatMessage : Packet
    {
        PacketFieldType::Byte player_id;
        PacketFieldType::String message;

        static constexpr PacketID packet_id = PacketID::ChatMessage;
        static constexpr std::size_t packet_size = 66;

        DECLARE_PACKET_READ_METHOD(PacketChatMessage);

        PacketChatMessage() = default;
        PacketChatMessage(std::string_view msg)
            : Packet { packet_id }
            , player_id{ 0xFF }
            , message{ msg.data(), msg.size() }
        { }

        bool serialize(io::IoEventData&) const;
    };

    struct PacketDisconnectPlayer : Packet
    {
        PacketFieldType::String reason;

        static constexpr PacketID packet_id = PacketID::DisconnectPlayer;
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

        constexpr static std::size_t packet_size = 2;

        PacketSetPlayerID(unsigned a_player_id)
            : Packet{ PacketID::SetPlayerID }
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

        static inline void read_scalar(std::byte*& buf, PacketFieldType::Byte &value)
        {
            value = *reinterpret_cast<PacketFieldType::Byte*>(buf++);
        }

        static inline void read_scalar(std::byte*& buf, PacketFieldType::Short& value)
        {
            value = _byteswap_ushort(*reinterpret_cast<PacketFieldType::Short*>(buf));
            buf += sizeof(PacketFieldType::Short);
        }

        static inline void read_scalar(std::byte*& buf, PacketFieldType::Int& value)
        {
            value = _byteswap_ulong(*reinterpret_cast<PacketFieldType::Int*>(buf));
            buf += sizeof(PacketFieldType::Int);
        }

        static void read_string(std::byte*& buf, PacketFieldType::String& value)
        {
            {
                std::uint16_t padding_size = 0;
                for (; padding_size < 64 && buf[63 - padding_size] == std::byte(' '); padding_size++)
                    buf[63 - padding_size] = std::byte(0);
                value.size = 64 - padding_size;
            }
            value.data = reinterpret_cast<const char*>(buf);
            buf += 64;
            * (buf - 1) = std::byte(0);
        }

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

        static void write_string(std::byte*& buf, std::string_view str);

        static void write_string(std::byte*&, const char*);

        static void write_string(std::byte*& buf, const std::byte* data, std::size_t data_size);

        static void write_coordinate(std::byte*&, util::Coordinate3D);

        static void write_orientation(std::byte*&, std::uint8_t yaw, std::uint8_t pitch);
    };
}