#pragma once

#include <utility>
#include <cstdlib>
#include <cstdint>
#include <cassert>

namespace net
{
	enum PacketID
	{
		KeepAlive = 0,
		LoginRequest = 1,
		Handshake = 2,
		ChatMessage = 3,
		TimeUpdate = 4,
		EntityEquipment = 5,
		SpawnPosition = 6,
		UseEntity = 7,
		UpdateHealth = 8,
		Respawn = 9,
		Player = 0xa,
		PlayerPosition = 0xb,
		PlayerLook = 0xc,
		PlayerPositionAndLook = 0xd,
		PlayerDigging = 0xe,
		PlayerBlockPlacement = 0xf,
		HeldItemChange = 0x10,
		UseBed = 0x11,
		Animation = 0x12,
		EntityAction = 0x13,
		SpawnNamedEntity = 0x14,
		SpawnDroppedItem = 0x15,
		CollectItem = 0x16,
		SpawnObject = 0x17,
		SpawnMob = 0x18,
		SpawnPainting = 0x19,
		SpawnExperienceOrb = 0x1a,
		EntityVelocity = 0x1c,
		DestoryEntity = 0x1d,
		Entity = 0x1e,
		EntityRelativeMove = 0x1f,
		EntityLook = 0x20,
		EntityLookAndRelativeMove = 0x21,
		EntityTeleport = 0x22,
		EntityHeadLook = 0x23,
		EntityStatus = 0x26,
		AttachEntity = 0x27,
		EntityMetadata = 0x28,
		EntityEffect = 0x29,
		RemoveEntityEffect = 0x2a,
		SetExperience = 0x2b,
		ChunkAllocation = 0x32,
		ChunkData = 0x33,
		MultiBlockChange = 0x34,
		BlockChange = 0x35,
		BlockAction = 0x36,
		Explosion = 0x3c,
		SoundOrParticleEffect = 0x3d,
		ChangeGameState = 0x46,
		Thunderbolt = 0x47,
		OpenWindow = 0x64,
		CloseWindow = 0x65,
		ClickWindow = 0x66,
		SetSlot = 0x67,
		SetWindowsItems = 0x68,
		UpdateWindowProperty = 0x69,
		ConfirmTransaction = 0x6a,
		CreativeInventoryAction = 0x6b,
		EnchantItem = 0x6c,
		UpdateSign = 0x82,
		ItemData = 0x83,
		UpdateTileEntity = 0x84,
		IncrementStatisic = 0xc8,
		PlayerListItem = 0xc9,
		PlayerAbilities = 0xca,
		PluginMessage = 0xfa,
		ServerListPing = 0xfe,
		Kick = 0xff,
		INVALID = 0xffff,
	};

	enum PacketParseError
	{
		SUCCESS,
		INVALID_PACKET_ID,
		LACK_OF_SIZE,
	};

	namespace PacketFieldType
	{
		using Short = std::int16_t;
		using Int = std::int32_t;

		struct String
		{
			Short size = 0;
			const char *data = nullptr;
		};
	};


	struct Packet
	{
		PacketID  id;
	};

	struct PacketKeepAlive : Packet
	{
		PacketFieldType::Int keep_alive_id;
	};

	struct PacketHandshake : Packet
	{
		PacketFieldType::String username_and_host;

		static std::byte* parse(std::byte* buf_start, std::byte* buf_end, Packet*);
	};

	struct PacketStructure
	{
		consteval static std::size_t size_of_max_packet_struct()
		{
			return std::max(sizeof(PacketKeepAlive), sizeof(PacketHandshake));
		}

		static std::size_t parse_packet(std::byte* buf_start, std::byte* buf_end, Packet*);

		static bool validate_packet(Packet*);
	};
}