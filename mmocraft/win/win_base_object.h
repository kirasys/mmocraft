#pragma once
#include <Windows.h>

namespace win
{
    template <typename T>
    class WinBaseObject
    {
    public:
        virtual T get_handle() const = 0;

        virtual bool is_valid() const = 0;

        virtual void close() noexcept = 0;
    private:

    };
}