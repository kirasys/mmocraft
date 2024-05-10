#include "pch.h"
#include "packet.h"

#include <array>

#define OVERFLOW_CHECK(buf_start, buf_end, size) \
		if (buf_start + size > buf_end) return nullptr;

#define PARSE_FIELD(buf_start, out, type) \
		*out = *reinterpret_cast<type*>(buf_start); \
		buf_start += sizeof(type);

#define PARSE_STRING_FIELD(buf_start, buf_end, out) \
		OVERFLOW_CHECK(buf_start, buf_end, sizeof(PacketFieldType::Short)) \
		PARSE_FIELD(buf_start, &(out)->size, PacketFieldType::Short) \
		OVERFLOW_CHECK(buf_start, buf_end, (out)->size); \
		(out)->data = reinterpret_cast<const char*>(buf_start); \
		buf_start += (out)->size;

namespace
{
	struct PacketStaticData
	{
		std::byte* (*parse)(std::byte*, std::byte*, net::Packet*) = nullptr;
	};

	constinit const std::array<PacketStaticData, 0x100> packet_static_data_by_id = [] {
		using namespace net;
		std::array<PacketStaticData, 0x100> arr{};
		arr[PacketID::Handshake] = { &PacketHandshake::parse};
		return arr;
	}();
}

namespace net
{
	/* Common Packet Static Methods */

	std::size_t PacketStructure::parse_packet(std::byte* buf_start, std::byte* buf_end, Packet* out_packet)
	{
		assert(buf_start < buf_end);

		// parse packet common header.
		auto packet_id = PacketID(*buf_start);
		auto packet_parser = *packet_static_data_by_id[packet_id].parse;

		out_packet->id = packet_parser ? packet_id : PacketID::INVALID;
		if (out_packet->id == PacketID::INVALID)
			return 1; // stop parsing invalid packet.

		// parse concrete packet structure.
		if (auto new_buf_start = (*packet_parser)(buf_start + 1, buf_end, out_packet))
			return new_buf_start - buf_start;

		return 0; // insufficient packet data.
	}

	bool PacketStructure::validate_packet(Packet* packet)
	{
		if (packet->id == PacketID::INVALID)
			return false;

		return true;
	}

	/* Concrete Packet Static Methods */

	std::byte* PacketHandshake::parse(std::byte* buf_start, std::byte* buf_end, Packet* out_packet)
	{
		auto packet = static_cast<PacketHandshake*>(out_packet);
		PARSE_STRING_FIELD(buf_start, buf_end, &packet->username_and_host);
		return buf_start;
	}
}