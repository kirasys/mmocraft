#pragma once

#include "logging/error.h"

namespace net
{
    class ServerCore
    {
    public:
        enum State
        {
            Uninitialized,
            Initialized,
            Running,
            Stopped,
        };

        State state() const
        {
            return _state;
        }

        void set_state(State state)
        {
            _state = state;
        }

        bool is_stopped() const
        {
            return _state == State::Stopped;
        }

        error::ResultCode last_error() const
        {
            return last_error_code;
        }

        void set_last_error(error::ErrorCode code)
        {
            last_error_code = code;
        }

        virtual ~ServerCore() = default;

        virtual void start_network_io_service() = 0;

    private:
        State _state = Uninitialized;
        error::ResultCode last_error_code;
    };
}