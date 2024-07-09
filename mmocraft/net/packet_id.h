#pragma once

namespace net
{
    enum PacketID
    {
        // Client <-> Server
        Handshake = 0,
        SetPlayerPosition = 8,
        ChatMessage = 0xD,

        // Client -> Server
        SetBlockClient = 5,

        // Server -> Client
        Ping = 1,
        LevelInitialize = 2,
        LevelDataChunk = 3,
        LevelFinalize = 4,
        SetBlockServer = 6,
        SpawnPlayer = 7,
        UpdatePlayerPosition = 9,
        UpdatePlayerCoordinate = 0xA,
        UpdatePlayerOrientation = 0xB,
        DespawnPlayer = 0xC,
        DisconnectPlayer = 0xE,
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

        /* Custom Protocol */
        SetPlayerID = 0x37,
        ExtMessage = 0x38,

        // Indicate size of the enum class.
        SIZE,

        // Invalid packet ID.
        INVALID = 0xFF,

    };
}