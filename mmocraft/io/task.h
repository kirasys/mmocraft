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

        State status() const
        {
            return _state;
        }

        bool transit_state(State old_state, State new_state)
        {
            if (_state == old_state) {
                _state = new_state;
                return true;
            }
            return false;
        }

        virtual void invoke_handler(ULONG_PTR task_handler_inst) = 0;

        virtual bool exists() const = 0;
    };
    
    template <typename HandlerClass>
    class SimpleTask : public io::Task
    {
    public:
        using handler_type = void (HandlerClass::* const)();

        SimpleTask(handler_type handler, HandlerClass* handler_inst = nullptr)
            : _handler{ handler }
            , _handler_inst{ handler_inst }
        { }

        virtual void invoke_handler(ULONG_PTR task_handler_inst) override
        {
            std::invoke(_handler,
                _handler_inst ? *_handler_inst : *reinterpret_cast<HandlerClass*>(task_handler_inst)
            );

            transit_state(State::Processing, State::Unused);
        }

        virtual bool exists() const
        {
            return status() == State::Unused;
        }
    private:
        handler_type _handler;
        HandlerClass* _handler_inst = nullptr;
    };
    
}