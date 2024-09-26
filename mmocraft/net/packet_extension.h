#pragma once

#include <string_view>
#include "packet.h"

namespace net
{
    bool is_cpe_support(std::string_view, int version);
    net::PacketID cpe_index_of(std::string_view);

    struct PacketExtInfo : Packet
    {
        PacketFieldType::String app_name;
        PacketFieldType::Short extension_count;

        static constexpr PacketID packet_id = PacketID::ExtInfo;
        constexpr static std::size_t packet_size = 67;

        PacketExtInfo() = default;

        PacketExtInfo(const std::byte* data)
            : Packet{ packet_id }
        { 
            parse(data);
        }

        bool serialize(io::IoEventData&) const;

        void parse(const std::byte* buf_start);
    };

    struct PacketExtEntry : Packet
    {
        PacketFieldType::String extenstion_name;
        PacketFieldType::Int version;

        static constexpr PacketID packet_id = PacketID::ExtEntry;
        constexpr static std::size_t packet_size = 69;

        PacketExtEntry() = default;

        PacketExtEntry(const std::byte* data)
            : Packet{ packet_id }
        {
            parse(data);
        }

        bool serialize(io::IoEventData&) const;

        void parse(const std::byte* buf_start);
    };

    struct PacketTwoWayPing : Packet
    {
        static constexpr PacketID packet_id = PacketID::TwoWayPing;
        static constexpr std::size_t packet_size = 4;
    };

    enum MessageType
    {
        Chat = 0,
        Status1 = 1,
        Status2 = 2,
        Status3 = 3,
        BottomRight1 = 11,
        BottomRight2 = 12,
        BottomRight3 = 13,
        Announcement = 100,
    };

    struct PacketExtMessage : Packet
    {
        MessageType msg_type;
        PacketFieldType::String message;

        static constexpr PacketID packet_id = PacketID::ExtMessage;
        static constexpr std::size_t packet_size = 66;

        PacketExtMessage(MessageType type, const char* msg)
            : Packet{ packet_id }
            , msg_type{ type }
            , message{ msg, std::strlen(msg) }
        { }

        bool serialize(io::IoEventData&) const;
    };

    struct PacketExtPing : Packet
    {
        PacketFieldType::UInt64 request_time = 0;

        static constexpr PacketID packet_id = PacketID::ExtPing;
        static constexpr std::size_t packet_size = 9;

        PacketExtPing() = default;

        PacketExtPing(const std::byte* data)
            : Packet{ packet_id }
        {
            parse(data);
        }

        void set_request_time()
        {
            request_time = util::current_time_ns();
        }

        std::size_t get_rtt_ns() const
        {
            return util::current_time_ns() - request_time;
        }

        void parse(const std::byte* buf_start);

        bool serialize(io::IoEventData&) const;
    };
}