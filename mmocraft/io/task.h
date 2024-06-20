#pragma once

#include "net/connection_key.h"
#include "logging/logger.h"
#include "util/time_util.h"

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

        Task(std::size_t interval) : interval_ms{ interval }
        { }

        virtual ~Task() = default;

        State status() const
        {
            return _state;
        }

        virtual bool ready() const
        {
            return _state == State::Unused && cooldown_at < util::current_monotonic_tick();
        }

        void set_state(State state)
        {
            if (state == State::Processing)
                cooldown_at = interval_ms + util::current_monotonic_tick();

            _state = state;
        }

        bool transit_state(State old_state, State new_state)
        {
            _state = _state == old_state ? new_state : old_state;
            return _state == new_state;
        }

        virtual void invoke_handler(ULONG_PTR task_handler_inst) = 0;

    private:
        State _state = Unused;
        std::size_t interval_ms = 0;
        std::size_t cooldown_at = 0;
    };
    
    template <typename HandlerClass>
    class SimpleTask : public io::Task
    {
    public:
        using handler_type = void (HandlerClass::* const)();

        SimpleTask(handler_type handler, HandlerClass* handler_inst, std::size_t interval_ms = 0)
            : io::Task{ interval_ms }
            , _handler{ handler }
            , _handler_inst{ handler_inst }
        { }

        virtual void invoke_handler(ULONG_PTR task_handler_inst) override
        {
            try {
                std::invoke(_handler,
                    _handler_inst ? *_handler_inst : *reinterpret_cast<HandlerClass*>(task_handler_inst)
                );
            }
            catch (...) {
                CONSOLE_LOG(error) << "Unexpected error occured at simple task handler";
            }

            set_state(State::Unused);
        }

    private:
        handler_type _handler;
        HandlerClass* _handler_inst = nullptr;
    };
    
}