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

		if (packet.password.size == 0 || packet.password.size > PacketFieldConstraint::max_password_length)
			return error::PACKET_HANSHAKE_IMPROPER_PASSWORD_LENGTH;

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