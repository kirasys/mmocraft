#pragma once

namespace net
{
    namespace message_id {
        enum value {
            invalid = 0,

            // Common message
            ping,
            server_announcement,

            // Chat server message
            packet_chat_message,

            // Router server message
            get_config,
            get_server_address,

            // Login server message
            packet_handshake,
            player_logout,

            size,
        };
    }
}