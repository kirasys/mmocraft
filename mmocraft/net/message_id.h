#pragma once

namespace net
{
    enum MessageID
    {
        Invalid_MessageID,

        Common_ServerAnnouncement,

        Chat_Ping,
        Chat_PacketMessage,

        Router_Pong,
        Router_GetConfig,
        Router_FetchServer,

        Login_PacketHandshake,
        Login_PlayerLogout,

        // Indicate size of the enum class.
        Size_of_MessageID,
    };
}