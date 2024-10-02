#pragma once

#include <string_view>
#include "logging/error.h"

namespace net
{
    class ServerCore
    {
    public:
        enum class State
        {
            uninitialized,
            initialized,
            running,
            stop,
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
            return _state == State::stop;
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

        virtual void start_network_io_service(std::string_view ip, int port, std::size_t num_of_event_threads) = 0;

    private:
        State _state = State::uninitialized;
        error::ResultCode last_error_code;
    };
}