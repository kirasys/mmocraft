#pragma once

namespace net
{
    // Protocol for communication with the chat server.
    enum MessageID
    {
        Invalid_MessageID,

        Chat_Ping,
        Chat_Packet,

        Router_Pong,
        Router_GetConfig,
        Router_ServerAnnouncement,
        Router_FetchServer,

        // Indicate size of the enum class.
        Size_of_MessageID,
    };
}