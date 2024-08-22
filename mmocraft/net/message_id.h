#pragma once

namespace net
{
    enum MessageID
    {
        Invalid_MessageID,

        Chat_Ping,
        Chat_PacketMessage,

        Router_Pong,
        Router_GetConfig,
        Router_ServerAnnouncement,
        Router_FetchServer,

        Login_PacketHandshake,

        // Indicate size of the enum class.
        Size_of_MessageID,
    };
}