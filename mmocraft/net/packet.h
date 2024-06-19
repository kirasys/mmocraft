#pragma once

#include <utility>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <string_view>

#include "io/io_event.h"
#include "logging/error.h"
#include "game/player.h"
#include "util/compressor.h"
#include "util/math.h"

#define DECLARE_PACKET_READ_METHOD(packet_type) \
    static std::byte* parse(std::byte* buf_start, std::byte* buf_end, net::Packet*); \
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
    enum PacketID
    {
        // Client <-> Server
        Handshake = 0,
        PositionAndOrientation = 8,
        Message = 0xD,

        // Client -> Server
        SetBlockClient = 5,

        // Server -> Client
        Ping = 1,
        LevelInitialize = 2,
        LevelDataChunk = 3,
        LevelFinalize = 4,
        SetBlockServer = 6,
        SpawnPlayer = 7,
        PositionAndOrientationUpdate = 9,
        PositionUpdate = 0xA,
        OrientationUpdate = 0xB,
        DespawnPlayer = 0xC,
        DisconnectPlayer = 0xE,
        UpdateUserType = 0xF,

        // CPE
        ExtInfo = 0x10,
        ExtEntry = 0x11,
        ClickDistance = 0x12,
        CustomBlocks = 0x13,
        HeldBlock = 0x14,
        TextHotKey = 0x15,
        ExtAddPlayerName = 0x16,
        ExtAddEntity2 = 0x21,
        ExtRemovePlayerName = 0x18,
        EvnSetColor = 0x19,
        MakeSelection = 0x1A,
        RemoveSelection = 0x1B,
        BlockPermissions = 0x1C,
        ChangeModel = 0x1D,
        EnvSetWeatherType = 0x1F,
        HackControl = 0x20,
        PlayerClicked = 0x22,
        DefineBlock = 0x23,
        RemoveBlockDefinition = 0x24,
        DefineBlockExt = 0x25,
        BulkBlockUpdate = 0x26,
        SetTextColor = 0x27,
        SetMapEnvUrl = 0x28,
        SetMapEnvProperty = 0x29,
        SetEntityProperty = 0x2A,
        TwoWayPing = 0x2B,
        SetInventoryOrder = 0x2C,
        SetHotbar = 0x2D,
        SetSpawnpoint = 0x2E,
        VelocityControl = 0x2F,
        DefineEffect = 0x30,
        SpawnEffect = 0x31,
        DefineModel = 0x32,
        DefineModelPart = 0x33,
        UndefineModel = 0x34,
        ExtEntityTeleport = 0x36,

        /* Custom Protocol */
        SetPlayerID = 0x37,

        INVALID = 0xFF,
        
        // Indicate size of the enum class.
        SIZE,
    };

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

        struct String
        {
            const char *data = nullptr;
            std::size_t size = 0;
            static constexpr unsigned size_with_padding = 64;
        };
    };

    namespace PacketFieldConstraint
    {
        static constexpr unsigned max_username_length = 16;
        static constexpr unsigned max_password_length = 32;
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
            PacketFieldType::Byte unused;
            PacketFieldType::Byte user_type;
        };

        DECLARE_PACKET_READ_METHOD(PacketHandshake);

        PacketHandshake(const std::string& a_server_name, const std::string& a_motd, net::UserType a_user_type)
            : Packet{ PacketID::Handshake }
            , server_name{ a_server_name.data(), a_server_name.size() }
            , motd{ a_motd.data(), a_motd.size() }
            , protocol_version{ 7 }
            , user_type{ PacketFieldType::Byte(a_user_type) }
        { }

        constexpr static std::size_t packet_size = 131;

        bool serialize(io::IoEventData&) const;
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
        static constexpr unsigned chunk_size = 1024;
        std::size_t max_chunk_count = 0;
        PacketFieldType::Short x;
        PacketFieldType::Short y;
        PacketFieldType::Short z;

        PacketLevelDataChunk(char* block_data, unsigned block_data_size, PacketFieldType::Short, PacketFieldType::Short, PacketFieldType::Short);

        std::size_t serialize(std::unique_ptr<std::byte[]>&);
        
    private:
        util::Compressor compressor;
    };

    struct PacketSetBlock : Packet
    {
        PacketFieldType::Short x;
        PacketFieldType::Short y;
        PacketFieldType::Short z;
        PacketFieldType::Byte mode;
        PacketFieldType::Byte block_type;

        DECLARE_PACKET_READ_METHOD(PacketSetBlock);
    };

    struct PacketSpawnPlayer : Packet
    {
        PacketSpawnPlayer() 
            : Packet{ PacketID::SpawnPlayer }
        { }

        constexpr static std::size_t packet_size = 74;

        static std::size_t serialize(const std::vector<game::Player*>&, const std::vector<game::Player*>&, std::unique_ptr<std::byte[]>&);

    };
    
    struct PacketDisconnectPlayer : Packet
    {
        PacketFieldType::String reason;

        PacketDisconnectPlayer(std::string_view a_reason)
            : Packet{ PacketID::DisconnectPlayer }
            , reason{ a_reason.data(), a_reason.size() }
        { }

        constexpr static std::size_t packet_size = 65;

        bool serialize(io::IoEventData&) const;
    };

    struct PacketSetPlayerID : Packet
    {
        PacketFieldType::Byte player_id;

        PacketSetPlayerID(unsigned a_player_id)
            : Packet{ PacketID::SetPlayerID }
            , player_id(PacketFieldType::Byte(a_player_id))
        { }

        constexpr static std::size_t packet_size = 2;

        bool serialize(io::IoEventData&) const;
    };

    struct PacketStructure
    {
        consteval static std::size_t max_size_of_packet_struct()
        {
            return sizeof(PacketHandshake);
        }

        static auto parse_packet(std::byte* buf_start, std::byte* buf_end, Packet*) -> std::pair<std::uint32_t, error::ResultCode>;

        static void write_byte(std::byte*&, PacketFieldType::Byte);

        static void write_short(std::byte*&, PacketFieldType::Short);

        static void write_string(std::byte*&, const PacketFieldType::String&);

        static void write_string(std::byte*&, const char*);

        static void write_position(std::byte*&, util::Coordinate3D);

        static void write_orientation(std::byte*&, std::uint8_t yaw, std::uint8_t pitch);
    };
}