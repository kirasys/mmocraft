#pragma once

#include <coroutine>
#include <optional>

namespace io
{
    template <typename T> struct AsyncTask;

    struct TaskPromiseBase
    {
        std::suspend_always initial_suspend() { return {}; }

        struct FinalAwaitable
        {
            std::coroutine_handle<> _cont = nullptr;

            bool await_ready() const noexcept
            { 
                return _cont == nullptr;
            }

            void await_suspend(std::coroutine_handle<>) noexcept
            {
                _cont.resume();
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

    private:
        std::coroutine_handle<> _cont = nullptr;
    };

    template <typename T>
    struct TaskPromise : TaskPromiseBase
    {
        AsyncTask<T> get_return_object() noexcept;

        template <typename Type>
        void return_value(Type&& value)
        {
            _value = std::forward<Type>(value);
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
                    _coroutine.resume(); // Resume initial suspend.
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
            // Destory only at final suspsend.
            if (_coroutine.done())
                _coroutine.destroy();
        }

        void start() const
        {
            _coroutine.resume();
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