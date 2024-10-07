#include "pch.h"
#include "error.h"

#include <array>

namespace
{
    constinit const std::array<const char*, 0x1000> error_messages = [] {
        using namespace error;
        std::array<const char*, 0x1000> arr{};

        // Network
        arr[error::code::network::client_connection_limit]   = "client_connection_limit";

        // Database
        arr[error::code::database::alloc_environment_handle] = "alloc_environment_handle";
        arr[error::code::database::alloc_connection_handle]  = "alloc_connection_handle";
        arr[error::code::database::alloc_statement_handle]   = "alloc_statement_handle";
        arr[error::code::database::set_attribute_version]	 = "set_attribute_version";
        arr[error::code::database::connect_server]           = "connect_server";

        // Packet parsing
        arr[error::code::packet::invalid_packet_id]        = "Unsupported Packet ID";
        arr[error::code::packet::unimplemented_packet_id]  = "Unimplemented Packet ID";
        arr[error::code::packet::insuffient_packet_data]   = "insuffient_packet_data";

        // Packet validation
        arr[error::code::packet::invalid_protocol_version] = "Unsupported protocol version";
        arr[error::code::packet::improper_username_length] = "Username must be 1 to 16 characters";
        arr[error::code::packet::improper_username_format] = "Username must be alphanumeric characters";
        arr[error::code::packet::improper_password_length] = "Password must be 1 to 32 characters";

        // Packet handling
        arr[error::code::packet::handle_error]    = "handle_error";
        arr[error::code::packet::handle_chat_message_error] = "Couldn't handle chat message. Please try reconnect.";

        // Packet result
        arr[error::code::packet::player_login_fail] = "Incorrect username or password";
        arr[error::code::packet::player_already_login] = "Already logged in";
        arr[error::code::packet::player_not_exist] = "player_not_exist";

        return arr;
    }();
}

namespace error
{
    const char* get_error_message(error::ErrorCode code)
    {
        return error_messages[code];
    }

    std::ostream& operator<<(std::ostream& os, error::ResultCode code)
    {
        if (auto msg = code.to_string())
            return os << msg;
        return os << int(code.to_error_code());
    }
}