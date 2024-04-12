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
		(out)->data = buf_start; \
		buf_start += (out)->size;

namespace
{
	struct PacketStaticData
	{
		std::uint8_t* (*parse)(std::uint8_t*, std::uint8_t*, net::Packet*) = nullptr;
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

	std::size_t PacketStructure::parse_packet(std::uint8_t* buf_start, std::uint8_t* buf_end, Packet* out_packet)
	{
		assert(buf_start < buf_end);

		auto packet_id = PacketID(*buf_start);
		auto parser = *packet_static_data_by_id[packet_id].parse;

		out_packet->id = parser ? packet_id : PacketID::INVALID;
		if (!parser)
			return 1; // stop parsing invalid packet.

		if (auto new_buf_start = (*parser)(buf_start + 1, buf_end, out_packet))
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

	std::uint8_t* PacketHandshake::parse(std::uint8_t* buf_start, std::uint8_t* buf_end, Packet* out_packet)
	{
		auto packet = static_cast<PacketHandshake*>(out_packet);
		PARSE_STRING_FIELD(buf_start, buf_end, &packet->username_and_host);
		return buf_start;
	}
}