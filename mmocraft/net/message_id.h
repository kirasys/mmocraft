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
            chat_command,

            // Router server message
            fetch_config,
            fetch_server_address,

            // Login server message
            packet_handshake,
            player_logout,

            size,
        };
    }
}