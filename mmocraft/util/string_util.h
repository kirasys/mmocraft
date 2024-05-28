#pragma once

#include <algorithm>
#include <cctype>
#include <cstring>

namespace util
{
    inline bool is_alphanumeric(const char* data, std::size_t size)
    {
        return std::all_of(data, data + size, [](unsigned char c) { return std::isalnum(c); });
    }
}