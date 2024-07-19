#include "pch.h"
#include "time_util.h"

namespace util
{
    void busy_wait(std::size_t ms)
    {
        auto start = current_monotonic_tick();
        while (current_monotonic_tick() - start < ms)
        {
            int j = 0;
            for (int i = 0; i < 1000; i++) { j++; }
        }
    }
}