#include "pch.h"
#include "packet_extension.h"

#include <unordered_map>

namespace
{
    const char* cpe_server_name = "mmocraft-cpe";

    struct CpeInfo
    {
        net::PacketID index;
        int version;
    };

    const std::unordered_map<std::string_view, CpeInfo> supported_cpe_map = {
        {"MessageTypes", {net::PacketID::ExtMessage, 1}}
    };
}

namespace net
{
    bool is_cpe_support(std::string_view ext_name, int version)
    {
        return supported_cpe_map.find(ext_name) != supported_cpe_map.end()
            && supported_cpe_map.at(ext_name).version == version;
    }

    net::PacketID cpe_index_of(std::string_view ext_name)
    {
        return supported_cpe_map.at(ext_name).index;
    }

    void PacketExtInfo::parse(const std::byte* buf_start)
    {
        buf_start++;
        PacketStructure::read_string(buf_start, this->app_name);
        PacketStructure::read_scalar(buf_start, this->extension_count);
    }

    void PacketExtEntry::parse(const std::byte* buf_start)
    {
        buf_start++;
        PacketStructure::read_string(buf_start, this->extenstion_name);
        PacketStructure::read_scalar(buf_start, this->version);
    }

    bool PacketExtInfo::serialize(io::IoEventData& event_data) const
    {
        std::byte buf[packet_size];

        std::byte* buf_start = buf;
        PacketStructure::write_byte(buf_start, packet_id);
        PacketStructure::write_string(buf_start, cpe_server_name);
        PacketStructure::write_short(buf_start, PacketFieldType::Short(supported_cpe_map.size()));

        return event_data.push(buf, sizeof(buf));
    }

    bool PacketExtEntry::serialize(io::IoEventData& event_data) const
    {
        for (auto& [ext_name, ext_info] : supported_cpe_map) {
            std::byte buf[packet_size];

            std::byte* buf_start = buf;
            PacketStructure::write_byte(buf_start, packet_id);
            PacketStructure::write_string(buf_start, ext_name);
            PacketStructure::write_int(buf_start, ext_info.version);

            event_data.push(buf, sizeof(buf));
        }
        
        return true; // TODO: handle insuffient send buffer.
    }

    bool PacketExtMessage::serialize(io::IoEventData& event_data) const
    {
        std::byte buf[packet_size];

        std::byte* buf_start = buf;
        PacketStructure::write_byte(buf_start, packet_id);
        PacketStructure::write_byte(buf_start, PacketFieldType::Byte(msg_type));
        PacketStructure::write_string(buf_start, message);

        return event_data.push(buf, sizeof(buf));
    }
}