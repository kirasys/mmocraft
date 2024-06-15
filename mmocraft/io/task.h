#pragma once

#include "net/connection_key.h"
#include "logging/error.h"

namespace io
{
    class Task
    {
    public:
        enum State
        {
            Unused,
            Processing,
            Failed,
        };

        virtual ~Task() = default;

        State _state = Unused;

        bool transit_state(State old_state, State new_state)
        {
            if (_state == old_state) {
                _state = new_state;
                return true;
            }
            return false;
        }

        virtual void invoke_handler(ULONG_PTR task_handler_inst) = 0;

        virtual void invoke_result_handler(void* result_handler_inst) = 0;

        virtual bool exists() const = 0;

        virtual bool result_exists() const = 0;

        virtual void push_result(net::ConnectionKey, error::ErrorCode) = 0;
    };
}