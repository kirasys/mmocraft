#pragma once

#include <coroutine>

namespace io
{
    template <typename T> struct AsyncTask;

    struct TaskPromiseBase
    {
        std::suspend_never initial_suspend() { return {}; }

        struct FinalAwaitable
        {
            std::coroutine_handle<> _cont = nullptr;

            bool await_ready() const noexcept
            { 
                return _cont == nullptr;
            }

            template <typename PromiseType>
            bool await_suspend(std::coroutine_handle<PromiseType> cont) noexcept
            {
                return cont.promise().resume();
            }

            constexpr void await_resume() noexcept {}
        };

        auto final_suspend() const noexcept
        {
            return FinalAwaitable{ _cont };
        }

        void unhandled_exception() {}

        void set_continuation(std::coroutine_handle<> cont)
        {
            _cont = cont;
        }

        bool resume() noexcept
        {
            return _cont != nullptr ? (_cont.resume(), true) : false;
        }

    private:
        std::coroutine_handle<> _cont = nullptr;
    };

    template <typename T>
    struct TaskPromise : TaskPromiseBase
    {
        AsyncTask<T> get_return_object() noexcept;

        void return_value(T&& value)
        {
            _value = std::forward<T>(value);
        }

        T&& result()
        {
            return std::move(_value);
        }

    private:
        T _value;
    };

    template<>
    struct TaskPromise<void> : TaskPromiseBase
    {
        AsyncTask<void> get_return_object() noexcept;

        void return_void() {}

    };

    template <typename T>
    struct AsyncTask
    {
        using promise_type = TaskPromise<T>;

        auto operator co_await()
        {
            struct Awaitable
            {
                std::coroutine_handle<promise_type> _coroutine;

                constexpr bool await_ready() const noexcept { return false; }

                void await_suspend(std::coroutine_handle<> cont)
                {
                    assert(_coroutine);
                    _coroutine.promise().set_continuation(cont);
                }

                decltype(auto) await_resume()
                {
                    assert(_coroutine);
                    return _coroutine.promise().result();
                }
            };
            return Awaitable{ _coroutine };
        }

        AsyncTask(std::coroutine_handle<promise_type> coroutine)
            : _coroutine(coroutine)
        {}

        ~AsyncTask()
        {
            if (_coroutine.done())
                _coroutine.destroy();
        }

    private:
        std::coroutine_handle<promise_type> _coroutine;
    };

    template <typename T>
    inline AsyncTask<T> TaskPromise<T>::get_return_object() noexcept
    {
        return { std::coroutine_handle<TaskPromise>::from_promise(*this) };
    }

    inline AsyncTask<void> TaskPromise<void>::get_return_object() noexcept
    {
        return { std::coroutine_handle<TaskPromise>::from_promise(*this) };
    }
}