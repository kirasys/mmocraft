#pragma once

#include <chrono>
#include <ctime>

namespace util
{
    enum Second
    { };

    enum MilliSecond
    { };

    inline std::time_t current_timestmap()
    {
        return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    }

    inline auto current_monotonic_tick()
    {
        return std::size_t(::GetTickCount64());
    }

    inline auto sleep_ms(std::size_t ms)
    {
        ::Sleep(DWORD(ms));
    }
}