#pragma once

namespace net
{
    enum MessageID
    {
        Invalid_MessageID,

        General_PacketHandle,

        Chat_Ping,

        Router_Pong,
        Router_GetConfig,
        Router_ServerAnnouncement,
        Router_FetchServer,

        // Indicate size of the enum class.
        Size_of_MessageID,
    };
}