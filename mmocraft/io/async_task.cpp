#include "pch.h"
#include "async_task.h"

namespace io
{
    template <typename T>
    AsyncTask<T> TaskPromise<T>::get_return_object() noexcept
    {
        return { std::coroutine_handle<TaskPromise>::from_promise(*this) };
    }

    AsyncTask<void> TaskPromise<void>::get_return_object() noexcept
    {
        return { std::coroutine_handle<TaskPromise>::from_promise(*this) };
    }
}