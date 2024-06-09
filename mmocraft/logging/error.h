#pragma once

#include <iostream>
#include <sstream>
#include <utility>
#include <source_location>

namespace error
{
    enum ErrorCode
    {
        SUCCESS,

        // Socket
        SOCKET_CREATE,
        SOCKET_BIND,
        SOCKET_LISTEN,
        SOCKET_ACCEPTEX_LOAD,
        SOCKET_ACCEPTEX,
        SOCKET_SEND,
        SOCKET_RECV,
        SOCKET_SETOPT,

        // IO Service
        IO_SERVICE_CREATE_COMPLETION_PORT,

        // Client Connection
        CLIENT_CONNECTION_CREATE,
        CLIENT_CONNECTION_FULL,

        // Database
        DATABASE_ALLOC_ENVIRONMENT_HANDLE,
        DATABASE_ALLOC_CONNECTION_HANDLE,
        DATABASE_ALLOC_STATEMENT_HANDLE,
        DATABASE_SET_ATTRIBUTE_VERSION,
        DATABASE_CONNECT,

        // Packet parsing
        PACKET_INVALID_ID,
        PACKET_UNIMPLEMENTED_ID,
        PACKET_INSUFFIENT_DATA,

        // Pakcet validation
        PACKET_INVALID_DATA,
        PACKET_HANSHAKE_INVALID_PROTOCOL_VERSION,
        PACKET_HANSHAKE_IMPROPER_USERNAME_LENGTH,
        PACKET_HANSHAKE_IMPROPER_USERNAME_FORMAT,
        PACKET_HANSHAKE_IMPROPER_PASSWORD_LENGTH,

        // Packet handling
        PACKET_HANDLE_ERROR,
        PACKET_HANDLE_SUCCESS,
        PACKET_HANDLE_DEFERRED,

        // Deferred packet result
        PACKET_RESULT_SUCCESS_LOGIN,
        PACKET_RESULT_FAIL_LOGIN,
        PACKET_RESULT_ALREADY_LOGIN,

        // Indicate size of the enum class.
        SIZE,
    };

    const char* get_error_message(error::ErrorCode);

    class ResultCode
    {
    public:
        inline ResultCode(error::ErrorCode code = error::SUCCESS)
            : error_code{ code }
        { }

        inline void reset()
        {
            error_code = error::SUCCESS;
        }

        inline bool is_success() const
        {
            return error_code == error::SUCCESS;
        }

        inline bool is_packet_handle_success() const
        {
            return error_code == error::SUCCESS
                || error_code == error::PACKET_INSUFFIENT_DATA
                || error_code == error::PACKET_HANDLE_DEFERRED;
        }

        inline bool is_login_success() const
        {
            return error_code == error::PACKET_RESULT_SUCCESS_LOGIN;
        }

        inline const char* to_string() const
        {
            return get_error_message(error_code);
        }

        inline const ErrorCode to_error_code() const
        {
            return error_code;
        }

    private:
        error::ErrorCode error_code;
    };

    std::ostream& operator<<(std::ostream&, ErrorCode);

    std::ostream& operator<<(std::ostream&, ResultCode);
}