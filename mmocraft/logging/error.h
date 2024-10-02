#pragma once

#include <iostream>
#include <sstream>
#include <utility>
#include <source_location>

namespace error
{
    using ErrorCode = int;

    namespace code
    {
        constexpr ErrorCode success = 0;
        constexpr ErrorCode invaild = 1;

        namespace network {
            constexpr ErrorCode client_connection_limit = 1000;
        };

        namespace database {
            constexpr ErrorCode alloc_environment_handle = 2001;
            constexpr ErrorCode alloc_connection_handle = 2002;
            constexpr ErrorCode alloc_statement_handle = 2003;
            constexpr ErrorCode set_attribute_version = 2004;
            constexpr ErrorCode connect_server = 2005;
        };

        namespace packet
        {
            constexpr ErrorCode invalid_packet_id = 3001;
            constexpr ErrorCode unimplemented_packet_id = 3002;
            constexpr ErrorCode insuffient_packet_data = 3003;

            constexpr ErrorCode invalid_protocol_version = 3101;
            constexpr ErrorCode improper_username_length = 3102;
            constexpr ErrorCode improper_username_format = 3103;
            constexpr ErrorCode improper_password_length = 3104;

            constexpr ErrorCode handle_error = 3201;
            constexpr ErrorCode handle_suucess = 3202;
            constexpr ErrorCode handle_deferred = 3203;

            constexpr ErrorCode handle_chat_message_error = 3204;

            constexpr ErrorCode player_login_fail = 3301;
            constexpr ErrorCode player_not_exist = 3302;
            constexpr ErrorCode player_already_login = 3303;;
        };
    }

    const char* get_error_message(error::ErrorCode);

    class ResultCode
    {
    public:
        inline ResultCode(error::ErrorCode err = error::code::success)
            : code{ err }
        { }

        inline void reset()
        {
            code = error::code::success;
        }

        inline bool is_success() const
        {
            return code == error::code::success;
        }

        inline bool is_packet_handle_success() const
        {
            return code == error::code::success
                || code == error::code::packet::insuffient_packet_data
                || code == error::code::packet::handle_deferred;
        }

        inline const char* to_string() const
        {
            return get_error_message(code);
        }

        inline const error::ErrorCode to_error_code() const
        {
            return code;
        }

    private:
        error::ErrorCode code;
    };

    std::ostream& operator<<(std::ostream&, error::ResultCode);
}