#include "pch.h"
#include "packet.h"

#include <array>

#define OVERFLOW_CHECK(buf_start, buf_end, size) \
        if (buf_start + size > buf_end) return nullptr;

#define PARSE_SCALAR_FIELD(buf_start, buf_end, out) \
        OVERFLOW_CHECK(buf_start, buf_end, sizeof(decltype(out))); \
        out = *reinterpret_cast<decltype(out)*>(buf_start); \
        buf_start += sizeof(decltype(out));

#define PARSE_STRING_FIELD(buf_start, buf_end, out) \
        OVERFLOW_CHECK(buf_start, buf_end, 64) \
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
        std::byte* (*parse)(std::byte*, std::byte*, net::Packet*) = nullptr;
        error::ErrorCode (*validate)(const net::Packet*) = nullptr;
    };

    constinit const std::array<PacketStaticData, 0x100> packet_metadata_db = [] {
        using namespace net;
        std::array<PacketStaticData, 0x100> arr{};
        arr[PacketID::Handshake] = { PacketHandshake::parse, PacketHandshake::validate };
        arr[PacketID::SetBlockClient] = { PacketSetBlock::parse, PacketSetBlock::validate };
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
        auto packet_parser = packet_metadata_db[packet_id].parse;

        out_packet->id = packet_parser ? packet_id : PacketID::INVALID;
        if (out_packet->id == PacketID::INVALID)
            return { 0, error::PACKET_INVALID_ID }; // stop parsing invalid packet.

        // parse and validate concrete packet structure.
        if (auto new_buf_start = packet_parser(buf_start + 1, buf_end, out_packet)) {
            auto packet_validater = packet_metadata_db[packet_id].validate;
            auto error_code = packet_validater ? packet_validater(out_packet) : error::SUCCESS;
            return { std::uint32_t(new_buf_start - buf_start), error_code };
        }

        return { 0, error::PACKET_INSUFFIENT_DATA }; // insufficient packet data.
    }

    void PacketStructure::write_byte(std::byte* &buf, PacketFieldType::Byte value)
    {
        *buf++ = std::byte(value);
    }

    void PacketStructure::write_short(std::byte* &buf, PacketFieldType::Short value)
    {
        *reinterpret_cast<decltype(value)*>(buf) = _byteswap_ushort(value);
        buf += sizeof(value);
    }

    void PacketStructure::write_string(std::byte* &buf, const PacketFieldType::String& str)
    {
        std::memcpy(buf, str.data, str.size);
        std::memset(buf + str.size, ' ', str.size_with_padding - str.size);
        buf += str.size_with_padding;
    }

    /* Concrete Packet Static Methods */

    std::byte* PacketHandshake::parse(std::byte* buf_start, std::byte* buf_end, Packet* out_packet)
    {
        auto packet = to_derived(out_packet);
        PARSE_SCALAR_FIELD(buf_start, buf_end, packet->protocol_version);
        PARSE_STRING_FIELD(buf_start, buf_end, packet->username);
        PARSE_STRING_FIELD(buf_start, buf_end, packet->password);
        PARSE_SCALAR_FIELD(buf_start, buf_end, packet->unused);
        return buf_start;
    }

    error::ErrorCode PacketHandshake::validate(const net::Packet* a_packet)
    {
        auto &packet = *to_derived(a_packet);

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

    bool PacketHandshake::serialize(io::IoEventData& event_data) const
    {
        if (size_of_serialized() > event_data.unused_size())
            return false;

        std::byte* buf_start = event_data.data();
        PacketStructure::write_byte(buf_start, id);
        PacketStructure::write_byte(buf_start, protocol_version);
        PacketStructure::write_string(buf_start, server_name);
        PacketStructure::write_string(buf_start, motd);
        PacketStructure::write_byte(buf_start, user_type);

        event_data.commit(buf_start - event_data.data());

        return true;
    }

    bool PacketLevelInit::serialize(io::IoEventData& event_data) const
    {
        if (size_of_serialized() > event_data.unused_size())
            return false;

        std::byte* buf_start = event_data.data();
        PacketStructure::write_byte(buf_start, id);
        event_data.commit(1);

        return true;
    }

    PacketLevelDataChunk::PacketLevelDataChunk
        (char* block_data, unsigned block_data_size, PacketFieldType::Short width, PacketFieldType::Short height, PacketFieldType::Short length)
        : Packet{ PacketID::LevelDataChunk }
        , compressor{ block_data, block_data_size }
        , x{ width }, y{ height }, z{ length }
    {
        max_chunk_count = (compressor.deflate_bound() - 1) / chunk_size + 1;
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

    std::byte* PacketSetBlock::parse(std::byte* buf_start, std::byte* buf_end, Packet* out_packet)
    {
        auto packet = to_derived(out_packet);
        PARSE_SCALAR_FIELD(buf_start, buf_end, packet->x);
        PARSE_SCALAR_FIELD(buf_start, buf_end, packet->y);
        PARSE_SCALAR_FIELD(buf_start, buf_end, packet->z);
        PARSE_SCALAR_FIELD(buf_start, buf_end, packet->mode);
        PARSE_SCALAR_FIELD(buf_start, buf_end, packet->block_type);
        return buf_start;
    }

    error::ErrorCode PacketSetBlock::validate(const net::Packet* a_packet)
    {
        auto& packet = *to_derived(a_packet);

        if (packet.mode != 0 && packet.mode != 1)
            return error::PACKET_INVALID_DATA;

        return error::SUCCESS;
    }

    bool PacketDisconnectPlayer::serialize(io::IoEventData& event_data) const
    {
        if (size_of_serialized() > event_data.unused_size())
            return false;

        std::byte* buf_start = event_data.data();
        PacketStructure::write_byte(buf_start, id);
        PacketStructure::write_string(buf_start, reason);
        
        event_data.commit(buf_start - event_data.data());

        return true;
    }
}