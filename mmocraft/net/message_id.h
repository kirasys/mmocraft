#pragma once

namespace net
{
    // Protocol for communication with the chat server.
    enum MessageID
    {
        Chat_Ping,
        Chat_Packet,

        Router_Pong,
        Router_GetConfig,
        Router_ListChatServer,

        // Indicate size of the enum class.
        SIZE,
    };
}