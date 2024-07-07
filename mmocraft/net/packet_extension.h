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

        PacketExtInfo()
            : Packet{ packet_id }
        { }

        bool serialize(io::IoEventData&) const;

        DECLARE_PACKET_READ_METHOD(PacketExtInfo);
    };

    struct PacketExtEntry : Packet
    {
        PacketFieldType::String extenstion_name;
        PacketFieldType::Int version;

        static constexpr PacketID packet_id = PacketID::ExtEntry;
        constexpr static std::size_t packet_size = 69;

        PacketExtEntry()
            : Packet{ packet_id }
        { }

        bool serialize(io::IoEventData&) const;

        DECLARE_PACKET_READ_METHOD(PacketExtEntry);
    };
}