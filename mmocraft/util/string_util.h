#pragma once

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <charconv>
#include <stdexcept>

namespace util
{
    inline bool is_alphanumeric(const char* data, std::size_t size)
    {
        return std::all_of(data, data + size, [](unsigned char c) { return std::isalnum(c); });
    }

    inline bool is_whitespace(char c)
    {
        return c == ' ' || (c >= 0x9 && c <= 0xD);
    }

    template <typename IntegerType>
    IntegerType to_integer(const char* cstr)
    {
        return static_cast<IntegerType>(std::atoi(cstr));
    }

    template <typename IntegerType>
    IntegerType to_integer(std::string_view str)
    {
        IntegerType value;

        if (std::from_chars(str.data(), str.data() + str.size(), value).ec != std::errc{})
            throw std::invalid_argument("Invalid integer format");

        return value;
    }
}