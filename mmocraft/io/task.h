#pragma once

#include "io/io_event.h"
#include "net/connection_key.h"
#include "logging/logger.h"
#include "util/time_util.h"

namespace io
{
    class Task : public io::Event
    {
    public:
        enum class State
        {
            unused,
            processing,
            failed,
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
            return _state == State::unused && cooldown_at < util::current_monotonic_tick();
        }

        void set_state(State state)
        {
            if (state == State::processing)
                cooldown_at = interval_ms + util::current_monotonic_tick();

            _state = state;
        }

        bool transit_state(State old_state, State new_state)
        {
            _state = _state == old_state ? new_state : old_state;
            return _state == new_state;
        }

        virtual void before_scheduling()
        {
            set_state(State::processing);
        }

    private:
        State _state = State::unused;
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

        virtual void on_event_complete(void* completion_key, DWORD transferred_bytes) override
        {
            std::invoke(_handler,
                _handler_inst ? *_handler_inst : *reinterpret_cast<HandlerClass*>(completion_key)
            );

            set_state(State::unused);
        }

    private:
        handler_type _handler;
        HandlerClass* _handler_inst = nullptr;
    };
    
}