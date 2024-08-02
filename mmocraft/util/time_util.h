#pragma once

#include <chrono>
#include <ctime>

#include "win/win_type.h"

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

    inline auto current_monotonic_tick32()
    {
        return std::size_t(::GetTickCount());
    }

    inline auto sleep_ms(std::size_t ms)
    {
        ::Sleep(DWORD(ms));
    }

    void busy_wait(std::size_t ms);
}