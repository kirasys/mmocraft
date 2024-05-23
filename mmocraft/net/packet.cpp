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
			for (;padding_size < 64 && buf_start[63-padding_size] == std::byte(' '); padding_size++) { } \
			(out).size = 64 - padding_size; \
		} \
		(out).data = reinterpret_cast<const char*>(buf_start); \
		buf_start += 64;

namespace
{
	struct PacketStaticData
	{
		std::byte* (*parse)(std::byte*, std::byte*, net::Packet*) = nullptr;
	};

	constinit const std::array<PacketStaticData, 0x100> packet_metadata_db = [] {
		using namespace net;
		std::array<PacketStaticData, 0x100> arr{};
		arr[PacketID::Handshake] = { &PacketHandshake::parse };
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
		auto packet_id = decltype(Packet::id)(*buf_start);
		auto packet_parser = *packet_metadata_db[packet_id].parse;

		out_packet->id = packet_parser ? packet_id : PacketID::INVALID;
		if (out_packet->id == PacketID::INVALID)
			return 1; // stop parsing invalid packet.

		// parse concrete packet structure.
		if (auto new_buf_start = (*packet_parser)(buf_start + 1, buf_end, out_packet))
			return new_buf_start - buf_start;

		return 0; // insufficient packet data.
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
		auto packet = static_cast<PacketHandshake*>(out_packet);
		PARSE_SCALAR_FIELD(buf_start, buf_end, packet->protocol_version);
		PARSE_STRING_FIELD(buf_start, buf_end, packet->username);
		PARSE_STRING_FIELD(buf_start, buf_end, packet->verification_key);
		PARSE_SCALAR_FIELD(buf_start, buf_end, packet->unused);
		return buf_start;
	}

	bool PacketKick::serialize(io::IoEventRawData& out_stream) const
	{
		if (size_of_serialized() > out_stream.unused_size())
			return false;

		std::byte* out_data = out_stream.data();
		PacketStructure::write_byte(out_data, id);
		PacketStructure::write_string(out_data, reason);

		out_stream.commit(size_of_serialized());
		return true;
	}
}