#pragma once

#include <utility>
#include <cstdlib>
#include <cstdint>
#include <cassert>

#include "io/io_event.h"

namespace net
{
	enum PacketID
	{
		// Client <-> Server
		Handshake = 0,
		PositionAndOrientation = 8,
		Message = 0xD,

		// Client -> Server
		SetBlockClient = 5,

		// Server -> Client
		Ping = 1,
		LevelInitialize = 2,
		LevelDataChunk = 3,
		LevelFinalize = 4,
		SetBlockServer = 6,
		SpawnPlayer = 7,
		PositionAndOrientationUpdate = 9,
		PositionUpdate = 0xA,
		OrientationUpdate = 0xB,
		DespawnPlayer = 0xC,
		Kick= 0xE,
		UpdateUserType = 0xF,

		// CPE
		ExtInfo = 0x10,
		ExtEntry = 0x11,
		ClickDistance = 0x12,
		CustomBlocks = 0x13,
		HeldBlock = 0x14,
		TextHotKey = 0x15,
		ExtAddPlayerName = 0x16,
		ExtAddEntity2 = 0x21,
		ExtRemovePlayerName = 0x18,
		EvnSetColor = 0x19,
		MakeSelection = 0x1A,
		RemoveSelection = 0x1B,
		BlockPermissions = 0x1C,
		ChangeModel = 0x1D,
		EnvSetWeatherType = 0x1F,
		HackControl = 0x20,
		PlayerClicked = 0x22,
		DefineBlock = 0x23,
		RemoveBlockDefinition = 0x24,
		DefineBlockExt = 0x25,
		BulkBlockUpdate = 0x26,
		SetTextColor = 0x27,
		SetMapEnvUrl = 0x28,
		SetMapEnvProperty = 0x29,
		SetEntityProperty = 0x2A,
		TwoWayPing = 0x2B,
		SetInventoryOrder = 0x2C,
		SetHotbar = 0x2D,
		SetSpawnpoint = 0x2E,
		VelocityControl = 0x2F,
		DefineEffect = 0x30,
		SpawnEffect = 0x31,
		DefineModel = 0x32,
		DefineModelPart = 0x33,
		UndefineModel = 0x34,
		ExtEntityTeleport = 0x36,

		INVALID = 0xFF,
	};

	namespace PacketFieldType
	{
		using Byte = std::uint8_t;
		using SByte = std::int8_t;
		using FByte = std::int8_t;
		using Short = std::int16_t;
		using UShort = std::uint16_t;
		using FShort = std::uint16_t;

		struct String
		{
			const char *data = nullptr;
			UShort size;
		};
	};


	struct Packet
	{
		PacketID  id;
	};

	struct PacketHandshake : Packet
	{
		PacketFieldType::Byte protocol_version;
		PacketFieldType::String username;
		PacketFieldType::String verification_key;
		PacketFieldType::Byte unused;

		static std::byte* parse(std::byte* buf_start, std::byte* buf_end, Packet*);
	};

	/*
	struct PacketKick : Packet
	{
		PacketKick(PacketFieldType::String a_reason)
			: Packet{ PacketID::Kick }
			, reason{ a_reason }
		{ }

		std::size_t size_of_serialized() const
		{
			return 1 + 2 + reason.size;
		}

		bool serialize(io::IoEventStreamData&) const;

		static std::byte* parse(std::byte* buf_start, std::byte* buf_end, Packet*);

		PacketFieldType::String reason;
	};
	*/

	struct PacketStructure
	{
		consteval static std::size_t size_of_max_packet_struct()
		{
			return sizeof(PacketHandshake);
		}

		static std::size_t parse_packet(std::byte* buf_start, std::byte* buf_end, Packet*);

		static bool validate_packet(Packet*);

		static void write_byte(std::byte* buf, PacketFieldType::Byte value)
		{
			buf[0] = std::byte(value);
		}

		static void write_short(std::byte* buf, PacketFieldType::Short value)
		{
			*reinterpret_cast<PacketFieldType::Short*>(buf) = _byteswap_ushort(value);
		}

		static void write_string(std::byte* buf, const PacketFieldType::String& str);
	};
}