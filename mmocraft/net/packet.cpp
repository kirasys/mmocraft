#include "pch.h"
#include "packet.h"

#include <array>
#include <cstring>

#include "net/packet_extension.h"
#include "game/block.h"
#include "game/player.h"

namespace
{
    struct PacketStaticData
    {
        std::size_t size = 0;
    };

    constinit const std::array<PacketStaticData, 0x100> protocol_db = [] {
        using namespace net;
        std::array<PacketStaticData, 0x100> arr{};
        arr[packet_type_id::handshake] = { PacketHandshake::packet_size };
        arr[packet_type_id::ping] = { PacketPing::packet_size};
        arr[packet_type_id::set_block_client] = { PacketSetBlockClient::packet_size };
        arr[packet_type_id::set_player_position] = { PacketSetPlayerPosition::packet_size };
        arr[packet_type_id::chat_message] = { PacketChatMessage::packet_size };
        arr[packet_type_id::ext_info] = { PacketExtInfo::packet_size };
        arr[packet_type_id::two_way_ping] = { PacketTwoWayPing::packet_size };
        arr[packet_type_id::ext_entry] = { PacketExtEntry::packet_size };
        arr[packet_type_id::ext_ping] = { PacketExtPing::packet_size };
        return arr;
    }();
}

namespace net
{
    /* Common Packet Static Methods */

    /*
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
    */

    std::pair<net::packet_type_id::value, int> PacketStructure::parse_packet(const std::byte* buf_start)
    {
        auto packet_id = decltype(Packet::id)(*buf_start);
        auto packet_size = int(protocol_db[packet_id].size);
        return { packet_size ? net::packet_type_id::value(packet_id) : net::packet_type_id::invalid, packet_size };
    }

    void PacketStructure::read_string(const std::byte*& buf, PacketFieldType::String& value)
    {
        {
            std::uint16_t padding_size = 0;
            for (; padding_size < 64 && buf[63 - padding_size] == std::byte(' '); padding_size++) { }
            
            value = { reinterpret_cast<const char*>(buf), std::size_t(64 - padding_size) };
        }
        buf += PacketFieldConstraint::max_string_length;
    }

    void PacketStructure::write_string(std::byte* &buf, const PacketFieldType::String& str)
    {
        auto size = std::min(str.size(), PacketFieldConstraint::max_string_length);
        std::memcpy(buf, str.data(), size);
        std::memset(buf + size, ' ', PacketFieldConstraint::max_string_length - size);
        buf += PacketFieldConstraint::max_string_length;
    }

    void PacketStructure::write_string(std::byte*& buf, const char* str)
    {
        auto size = std::min(std::strlen(str), PacketFieldConstraint::max_string_length);
        std::memcpy(buf, str, size);
        std::memset(buf + size, ' ', PacketFieldConstraint::max_string_length - size);
        buf += PacketFieldConstraint::max_string_length;
    }

    void PacketStructure::write_string(std::byte*& buf, const std::byte* data, std::size_t data_size)
    {
        auto size = std::min(data_size, PacketFieldConstraint::max_string_length);
        std::memcpy(buf, data, size);
        std::memset(buf + size, ' ', PacketFieldConstraint::max_string_length - size);
        buf += PacketFieldConstraint::max_string_length;
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

    error::ErrorCode PacketHandshake::validate()
    {
        if (protocol_version != 7)
            return error::code::packet::invalid_protocol_version;

        if (username.size() == 0 || username.size() > PacketFieldConstraint::max_username_length)
            return error::code::packet::improper_username_length;

        if (not util::is_alphanumeric(username))
            return error::code::packet::improper_username_format;

        if (password.size() > PacketFieldConstraint::max_password_length)
            return error::code::packet::improper_password_length;

        return error::code::success;
    }

    void PacketHandshake::parse(const std::byte* buf_start)
    {
        buf_start++;
        PacketStructure::read_scalar(buf_start, this->protocol_version);
        PacketStructure::read_string(buf_start, this->username);
        PacketStructure::read_string(buf_start, this->password);
        PacketStructure::read_scalar(buf_start, this->cpe_magic);
    }

    void PacketPing::parse(const std::byte* buf_start)
    {
        return;
    }

    void PacketSetBlockClient::parse(const std::byte* buf_start)
    {
        buf_start++;
        PacketStructure::read_scalar(buf_start, this->x);
        PacketStructure::read_scalar(buf_start, this->y);
        PacketStructure::read_scalar(buf_start, this->z);
        PacketStructure::read_scalar(buf_start, this->mode);
        PacketStructure::read_scalar(buf_start, this->block_id);
    }

    void PacketSetPlayerPosition::parse(const std::byte* buf_start)
    {
        buf_start++;
        PacketStructure::read_scalar(buf_start, this->player_id);
        PacketStructure::read_scalar(buf_start, this->player_pos.view.x);
        PacketStructure::read_scalar(buf_start, this->player_pos.view.y);
        PacketStructure::read_scalar(buf_start, this->player_pos.view.z);
        PacketStructure::read_scalar(buf_start, this->player_pos.view.yaw);
        PacketStructure::read_scalar(buf_start, this->player_pos.view.pitch);
    }

    void PacketChatMessage::parse(const std::byte* buf_start)
    {
        buf_start++;
        PacketStructure::read_scalar(buf_start, this->player_id);
        PacketStructure::read_string(buf_start, this->message);
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
        PacketStructure::write_byte(buf_start, packet_type_id::level_finalize);
        PacketStructure::write_short(buf_start, x);
        PacketStructure::write_short(buf_start, y);
        PacketStructure::write_short(buf_start, z);

        return buf_start - serialized_data.get();
    }

    bool PacketSetBlockClient::serialize(io::IoEventData& event_data) const
    {
        std::byte buf[packet_size];

        std::byte* buf_start = buf;
        PacketStructure::write_byte(buf_start, packet_id);
        PacketStructure::write_short(buf_start, x);
        PacketStructure::write_short(buf_start, y);
        PacketStructure::write_short(buf_start, z);
        PacketStructure::write_byte(buf_start, mode);
        PacketStructure::write_byte(buf_start, block_id);

        return event_data.push(buf, sizeof(buf));
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
                *buf_start++ = std::byte(net::packet_type_id::set_player_position);
                *buf_start++ = std::byte(player->game_id());
                net::PacketStructure::write_short(buf_start, latest_pos.view.x);
                net::PacketStructure::write_short(buf_start, latest_pos.view.y);
                net::PacketStructure::write_short(buf_start, latest_pos.view.z);
                *buf_start++ = std::byte(latest_pos.view.yaw);
                *buf_start++ = std::byte(latest_pos.view.pitch);
            }
            // relative move position
            else if (diff.raw_coordinate() && diff.raw_orientation()) {
                *buf_start++ = std::byte(net::packet_type_id::update_player_position);
                *buf_start++ = std::byte(player->game_id());
                *buf_start++ = std::byte(diff.view.x);
                *buf_start++ = std::byte(diff.view.y);
                *buf_start++ = std::byte(diff.view.z);
                *buf_start++ = std::byte(latest_pos.view.yaw);
                *buf_start++ = std::byte(latest_pos.view.pitch);
            }
            // relative move coordinate
            else if (diff.raw_coordinate()) {
                *buf_start++ = std::byte(net::packet_type_id::update_player_coordinate);
                *buf_start++ = std::byte(player->game_id());
                *buf_start++ = std::byte(diff.view.x);
                *buf_start++ = std::byte(diff.view.y);
                *buf_start++ = std::byte(diff.view.z);
            }
            // relative move orientation
            else if (diff.raw_orientation()) {
                *buf_start++ = std::byte(net::packet_type_id::update_player_orientation);
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
                PacketStructure::write_byte(buf_start, PacketFieldType::Byte(packet_type_id::spawn_player));
                PacketStructure::write_byte(buf_start, player->game_id());
                PacketStructure::write_string(buf_start, player->username());
                
                game::PlayerPosition last_player_pos = player->last_position().raw ? player->last_position() : player->spawn_position();
                PacketStructure::write_coordinate(buf_start, last_player_pos.coordinate());
                PacketStructure::write_orientation(buf_start, last_player_pos.yaw(), last_player_pos.pitch());
            }
        };

        serialize_position_packet(old_players);
        serialize_position_packet(new_players);

        return data_size;
    }

    std::size_t PacketDespawnPlayer::serialize(const std::vector<game::Player*>& players , std::unique_ptr<std::byte[]>& serialized_data)
    {
        auto data_size = players.size() * packet_size;
        serialized_data.reset(new std::byte[data_size]);

        std::byte* buf_start = serialized_data.get();

        for (auto player : players) {
            PacketStructure::write_byte(buf_start, packet_id);
            PacketStructure::write_byte(buf_start, player->game_id());
        }

        return data_size;
    }

    bool PacketChatMessage::serialize(io::IoEventData& event_data) const
    {
        std::byte buf[packet_size];

        std::byte* buf_start = buf;
        PacketStructure::write_byte(buf_start, packet_id);
        PacketStructure::write_byte(buf_start, player_id);
        PacketStructure::write_string(buf_start, message);

        return event_data.push(buf, sizeof(buf));
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
        : Packet{ packet_type_id::level_datachunk }
        , compressor{ block_data, block_data_size }
        , x{ width }, y{ height }, z{ length }
    {
        max_chunk_count = (compressor.deflate_bound() - 1) / chunk_size + 1;
    }
}