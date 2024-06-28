#include "pch.h"
#include "packet.h"

#include <array>
#include <cstring>

#include "game/block.h"
#include "game/player.h"

#define PARSE_BYTE_FIELD(buf_start, buf_end, out) \
        out = *reinterpret_cast<decltype(out)*>(buf_start); \
        buf_start += sizeof(decltype(out));

#define PARSE_SHORT_FIELD(buf_start, buf_end, out) \
        out = _byteswap_ushort(*reinterpret_cast<decltype(out)*>(buf_start)); \
        buf_start += sizeof(decltype(out));

#define PARSE_STRING_FIELD(buf_start, buf_end, out) \
        { \
            std::uint16_t padding_size = 0; \
            for (;padding_size < 64 && buf_start[63-padding_size] == std::byte(' '); padding_size++) \
                buf_start[63-padding_size] = std::byte(0); \
            (out).size = 64 - padding_size; \
        } \
        (out).data = reinterpret_cast<const char*>(buf_start); \
        buf_start += 64; \
        *(buf_start - 1) = std::byte(0); \

namespace
{
    struct PacketStaticData
    {
        void (*parse)(std::byte*, std::byte*, net::Packet*) = nullptr;
        error::ErrorCode (*validate)(const net::Packet*) = nullptr;
        std::size_t size = 0;
    };

    constinit const std::array<PacketStaticData, 0x100> protocol_db = [] {
        using namespace net;
        std::array<PacketStaticData, 0x100> arr{};
        arr[PacketID::Handshake] = { PacketHandshake::parse, PacketHandshake::validate, PacketHandshake::packet_size };
        arr[PacketID::SetBlockClient] = { PacketSetBlockClient::parse, nullptr, PacketSetBlockClient::packet_size };
        arr[PacketID::SetPlayerPosition] = { PacketSetPlayerPosition::parse, nullptr,  PacketSetPlayerPosition::packet_size };
        arr[PacketID::ChatMessage] = { PacketChatMessage::parse, nullptr, PacketChatMessage::packet_size };
        return arr;
    }();
}

namespace net
{
    /* Common Packet Static Methods */

    auto PacketStructure::parse_packet(std::byte* buf_start, std::byte* buf_end, Packet* out_packet)
        -> std::pair<std::uint32_t, error::ResultCode>
    {
        assert(buf_start < buf_end);

        // parse packet common header.
        auto packet_id = decltype(Packet::id)(*buf_start);

        if (std::size_t(buf_end - buf_start) < protocol_db[packet_id].size)
            return { 0, error::PACKET_INSUFFIENT_DATA };

        auto packet_parser = protocol_db[packet_id].parse;
        if (packet_parser == nullptr)
            return { 0, error::PACKET_INVALID_ID }; // stop parsing invalid packet.

        // parse and validate concrete packet structure.
        error::ErrorCode result = error::SUCCESS;

        out_packet->id = packet_id;
        packet_parser(buf_start + 1, buf_end, out_packet);
        if (auto packet_validater = protocol_db[packet_id].validate)
            result = packet_validater(out_packet);

        return { std::uint32_t(protocol_db[packet_id].size), result }; // insufficient packet data.
    }

    inline void PacketStructure::write_byte(std::byte* &buf, PacketFieldType::Byte value)
    {
        *buf++ = std::byte(value);
    }

    inline void PacketStructure::write_short(std::byte* &buf, PacketFieldType::Short value)
    {
        *reinterpret_cast<decltype(value)*>(buf) = _byteswap_ushort(value);
        buf += sizeof(value);
    }

    inline void PacketStructure::write_uint64(std::byte*& buf, std::uint64_t value)
    {
        *reinterpret_cast<decltype(value)*>(buf) = _byteswap_uint64(value);
        buf += sizeof(value);
    }

    void PacketStructure::write_string(std::byte* &buf, const PacketFieldType::String& str)
    {
        std::memcpy(buf, str.data, str.size);
        std::memset(buf + str.size, ' ', str.size_with_padding - str.size);
        buf += str.size_with_padding;
    }

    void PacketStructure::write_string(std::byte*& buf, const char* str)
    {
        auto size = std::min(std::strlen(str), std::size_t(64));
        std::memcpy(buf, str, size);
        std::memset(buf + size, ' ', PacketFieldType::String::size_with_padding - size);
        buf += PacketFieldType::String::size_with_padding;
    }

    void PacketStructure::write_string(std::byte*& buf, const std::byte* data, std::size_t data_size)
    {
        std::memcpy(buf, data, data_size);
        std::memset(buf + data_size, ' ', PacketFieldType::String::size_with_padding - data_size);
        buf += PacketFieldType::String::size_with_padding;
    }

    void PacketStructure::write_coordinate(std::byte*& buf, util::Coordinate3D pos)
    {
        PacketStructure::write_short(buf, pos.x);
        PacketStructure::write_short(buf, pos.y);
        PacketStructure::write_short(buf, pos.z);
    }

    void PacketStructure::write_orientation(std::byte*& buf, std::uint8_t yaw, std::uint8_t pitch)
    {
        PacketStructure::write_byte(buf, yaw);
        PacketStructure::write_byte(buf, pitch);
    }

    /* Concrete Packet Static Methods */

    error::ErrorCode PacketHandshake::validate(const net::Packet* a_packet)
    {
        auto& packet = *to_derived(a_packet);

        if (packet.protocol_version != 7)
            return error::PACKET_HANSHAKE_INVALID_PROTOCOL_VERSION;

        if (packet.username.size == 0 || packet.username.size > PacketFieldConstraint::max_username_length)
            return error::PACKET_HANSHAKE_IMPROPER_USERNAME_LENGTH;

        if (not util::is_alphanumeric(packet.username.data, packet.username.size))
            return error::PACKET_HANSHAKE_IMPROPER_USERNAME_FORMAT;

        if (packet.password.size > PacketFieldConstraint::max_password_length)
            return error::PACKET_HANSHAKE_IMPROPER_PASSWORD_LENGTH;

        return error::SUCCESS;
    }

    error::ErrorCode PacketSetBlockClient::validate(const net::Packet* a_packet)
    {
        return error::SUCCESS;
    }


    error::ErrorCode PacketSetPlayerPosition::validate(const net::Packet* a_packet)
    {
        return error::SUCCESS;
    }

    error::ErrorCode PacketChatMessage::validate(const net::Packet* a_packet)
    {
        return error::SUCCESS;
    }

    void PacketHandshake::parse(std::byte* buf_start, std::byte* buf_end, Packet* out_packet)
    {
        auto packet = to_derived(out_packet);
        PARSE_BYTE_FIELD(buf_start, buf_end, packet->protocol_version);
        PARSE_STRING_FIELD(buf_start, buf_end, packet->username);
        PARSE_STRING_FIELD(buf_start, buf_end, packet->password);
        PARSE_BYTE_FIELD(buf_start, buf_end, packet->unused);
    }

    void PacketSetBlockClient::parse(std::byte* buf_start, std::byte* buf_end, Packet* out_packet)
    {
        auto packet = to_derived(out_packet);
        PARSE_SHORT_FIELD(buf_start, buf_end, packet->x);
        PARSE_SHORT_FIELD(buf_start, buf_end, packet->y);
        PARSE_SHORT_FIELD(buf_start, buf_end, packet->z);
        PARSE_BYTE_FIELD(buf_start, buf_end, packet->mode);
        PARSE_BYTE_FIELD(buf_start, buf_end, packet->block_id);
    }

    void PacketSetPlayerPosition::parse(std::byte* buf_start, std::byte* buf_end, Packet* out_packet)
    {
        auto packet = to_derived(out_packet);
        PARSE_BYTE_FIELD(buf_start, buf_end, packet->player_id);
        PARSE_SHORT_FIELD(buf_start, buf_end, packet->player_pos.view.x);
        PARSE_SHORT_FIELD(buf_start, buf_end, packet->player_pos.view.y);
        PARSE_SHORT_FIELD(buf_start, buf_end, packet->player_pos.view.z);
        PARSE_BYTE_FIELD(buf_start, buf_end, packet->player_pos.view.yaw);
        PARSE_BYTE_FIELD(buf_start, buf_end, packet->player_pos.view.pitch);
    }

    void PacketChatMessage::parse(std::byte* buf_start, std::byte* buf_end, Packet* out_packet)
    {
        auto packet = to_derived(out_packet);
        PARSE_BYTE_FIELD(buf_start, buf_end, packet->player_id);
        PARSE_STRING_FIELD(buf_start, buf_end, packet->message);
    }

    bool PacketHandshake::serialize(io::IoEventData& event_data) const
    {
        std::byte buf[packet_size];

        std::byte* buf_start = buf;
        PacketStructure::write_byte(buf_start, id);
        PacketStructure::write_byte(buf_start, protocol_version);
        PacketStructure::write_string(buf_start, server_name);
        PacketStructure::write_string(buf_start, motd);
        PacketStructure::write_byte(buf_start, user_type);
       
        return  event_data.push(buf, sizeof(buf));
    }

    bool PacketLevelInit::serialize(io::IoEventData& event_data) const
    {
        std::byte buf[packet_size];

        std::byte* buf_start = buf;
        PacketStructure::write_byte(buf_start, id);
        
        return event_data.push(buf, sizeof(buf));
    }

    std::size_t PacketLevelDataChunk::serialize(std::unique_ptr<std::byte[]>& serialized_data)
    {
        auto data_capacity = max_chunk_count * (chunk_size + 4) + 7; // LevelData + LevelFinalize 
        serialized_data.reset(new std::byte[data_capacity]);

        std::byte* buf_start = serialized_data.get();

        auto remain_bytes = 0;
        while ((remain_bytes = compressor.deflate_n(buf_start + 3, chunk_size)), remain_bytes != chunk_size) {
            PacketStructure::write_byte(buf_start, id);

            // write chunk length.
            PacketStructure::write_short(buf_start, PacketFieldType::Short(chunk_size - remain_bytes));

            // write chunk data (already written)
            buf_start += chunk_size;

            // write percent complete
            PacketStructure::write_byte(buf_start, 0);
        }

        // add null padding
        std::memset(buf_start - remain_bytes - 1, 0, remain_bytes != chunk_size ? remain_bytes : 0);

        // write level finalize information.
        PacketStructure::write_byte(buf_start, PacketID::LevelFinalize);
        PacketStructure::write_short(buf_start, x);
        PacketStructure::write_short(buf_start, y);
        PacketStructure::write_short(buf_start, z);

        return buf_start - serialized_data.get();
    }

    bool PacketSetBlockServer::serialize(io::IoEventData& event_data) const
    {
        std::byte buf[packet_size];

        std::byte* buf_start = buf;
        PacketStructure::write_byte(buf_start, packet_id);
        PacketStructure::write_short(buf_start, block_pos.x);
        PacketStructure::write_short(buf_start, block_pos.y);
        PacketStructure::write_short(buf_start, block_pos.z);
        PacketStructure::write_byte(buf_start, block_id);

        return event_data.push(buf, sizeof(buf));
    }

    std::size_t PacketSetPlayerPosition::serialize(const std::vector<game::Player*>& players, std::unique_ptr<std::byte[]>& serialized_data)
    {
        auto max_data_size = players.size() * packet_size;
        serialized_data.reset(new std::byte[max_data_size]);

        std::byte* buf_start = serialized_data.get();

        for (auto player : players) {
            auto latest_pos = player->last_position();
            const auto diff = latest_pos - player->last_transferred_position();

            // Todo: it seem to be confusing to update player's state at serialization.
            //       move some other place.
            player->update_last_transferred_position(latest_pos);

            // absolute move position
            if (std::abs(diff.view.x) > 32 || std::abs(diff.view.y) > 32 || std::abs(diff.view.y) > 32) {
                *buf_start++ = std::byte(net::PacketID::SetPlayerPosition);
                *buf_start++ = std::byte(player->game_id());
                net::PacketStructure::write_short(buf_start, latest_pos.view.x);
                net::PacketStructure::write_short(buf_start, latest_pos.view.y);
                net::PacketStructure::write_short(buf_start, latest_pos.view.z);
                *buf_start++ = std::byte(latest_pos.view.yaw);
                *buf_start++ = std::byte(latest_pos.view.pitch);
            }
            // relative move position
            else if (diff.raw_coordinate() && diff.raw_orientation()) {
                *buf_start++ = std::byte(net::PacketID::UpdatePlayerPosition);
                *buf_start++ = std::byte(player->game_id());
                *buf_start++ = std::byte(diff.view.x);
                *buf_start++ = std::byte(diff.view.y);
                *buf_start++ = std::byte(diff.view.z);
                *buf_start++ = std::byte(latest_pos.view.yaw);
                *buf_start++ = std::byte(latest_pos.view.pitch);
            }
            // relative move coordinate
            else if (diff.raw_coordinate()) {
                *buf_start++ = std::byte(net::PacketID::UpdatePlayerCoordinate);
                *buf_start++ = std::byte(player->game_id());
                *buf_start++ = std::byte(diff.view.x);
                *buf_start++ = std::byte(diff.view.y);
                *buf_start++ = std::byte(diff.view.z);
            }
            // relative move orientation
            else if (diff.raw_orientation()) {
                *buf_start++ = std::byte(net::PacketID::UpdatePlayerOrientation);
                *buf_start++ = std::byte(player->game_id());
                *buf_start++ = std::byte(latest_pos.view.yaw);
                *buf_start++ = std::byte(latest_pos.view.pitch);
            }
        }

        return buf_start - serialized_data.get();
    }

    std::size_t PacketSpawnPlayer::serialize
        (const std::vector<game::Player*>& old_players, const std::vector<game::Player*>& new_players, std::unique_ptr<std::byte[]>& serialized_data)
    {
        auto data_size = (old_players.size() + new_players.size()) * packet_size;
        serialized_data.reset(new std::byte[data_size]);

        std::byte* buf_start = serialized_data.get();

        auto serialize_position_packet = [&buf_start](const std::vector<game::Player*>& players) {
            for (const auto* player : players) {
                PacketStructure::write_byte(buf_start, PacketFieldType::Byte(PacketID::SpawnPlayer));
                PacketStructure::write_byte(buf_start, player->game_id());
                PacketStructure::write_string(buf_start, player->player_name());
                
                game::PlayerPosition last_player_pos = player->last_position().raw ? player->last_position() : player->spawn_position();
                PacketStructure::write_coordinate(buf_start, last_player_pos.coordinate());
                PacketStructure::write_orientation(buf_start, last_player_pos.yaw(), last_player_pos.pitch());
            }
        };

        serialize_position_packet(old_players);
        serialize_position_packet(new_players);

        return data_size;
    }

    std::size_t PacketDespawnPlayer::serialize(const std::vector<game::PlayerID>& player_ids , std::unique_ptr<std::byte[]>& serialized_data)
    {
        auto data_size = player_ids.size() * packet_size;
        serialized_data.reset(new std::byte[data_size]);

        std::byte* buf_start = serialized_data.get();

        for (auto player_id : player_ids) {
            PacketStructure::write_byte(buf_start, packet_id);
            PacketStructure::write_byte(buf_start, player_id);
        }

        return data_size;
    }

    bool PacketDisconnectPlayer::serialize(io::IoEventData& event_data) const
    {
        std::byte buf[packet_size];

        std::byte* buf_start = buf;
        PacketStructure::write_byte(buf_start, id);
        PacketStructure::write_string(buf_start, reason);

        return event_data.push(buf, sizeof(buf));
    }

    bool PacketSetPlayerID::serialize(io::IoEventData& event_data) const
    {
        std::byte buf[packet_size];

        std::byte* buf_start = buf;
        PacketStructure::write_byte(buf_start, id);
        PacketStructure::write_byte(buf_start, player_id);

        return event_data.push(buf, sizeof(buf));
    }

    PacketLevelDataChunk::PacketLevelDataChunk
        (std::byte* block_data, unsigned block_data_size, PacketFieldType::Short width, PacketFieldType::Short height, PacketFieldType::Short length)
        : Packet{ PacketID::LevelDataChunk }
        , compressor{ block_data, block_data_size }
        , x{ width }, y{ height }, z{ length }
    {
        max_chunk_count = (compressor.deflate_bound() - 1) / chunk_size + 1;
    }
}