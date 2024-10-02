#pragma once

#include <string_view>
#include "packet.h"

namespace net
{
    bool is_cpe_support(std::string_view, int version);
    net::packet_type_id::value cpe_index_of(std::string_view);

    struct PacketExtInfo : Packet
    {
        PacketFieldType::String app_name;
        PacketFieldType::Short extension_count;

        static constexpr net::packet_type_id::value packet_id = packet_type_id::ext_info;
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

        static constexpr net::packet_type_id::value packet_id = packet_type_id::ext_entry;
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
        static constexpr net::packet_type_id::value packet_id = packet_type_id::two_way_ping;
        static constexpr std::size_t packet_size = 4;
    };

    enum class chat_message_type_id
    {
        chat = 0,
        status1 = 1,
        status2 = 2,
        status3 = 3,
        bottom_right1 = 11,
        bottom_right2 = 12,
        bottom_right3 = 13,
        announcement = 100,
    };

    struct PacketExtMessage : Packet
    {
        chat_message_type_id msg_type;
        PacketFieldType::String message;

        static constexpr net::packet_type_id::value packet_id = packet_type_id::ext_message;
        static constexpr std::size_t packet_size = 66;

        PacketExtMessage(chat_message_type_id type, const char* msg)
            : Packet{ packet_id }
            , msg_type{ type }
            , message{ msg, std::strlen(msg) }
        { }

        bool serialize(io::IoEventData&) const;
    };

    struct PacketExtPing : Packet
    {
        PacketFieldType::UInt64 request_time = 0;

        static constexpr net::packet_type_id::value packet_id = packet_type_id::ext_ping;
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