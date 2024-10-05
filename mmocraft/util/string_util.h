#pragma once

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <charconv>
#include <stdexcept>
#include <memory>

namespace util
{
    inline bool is_alphanumeric(const std::string_view str)
    {
        return std::all_of(str.begin(), str.end(), [](unsigned char c) { return std::isalnum(c); });
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

    template<std::size_t SIZE>
    void string_copy(char (&dst)[SIZE], std::string_view src)
    {
        static_assert(SIZE > 0);
        std::memcpy(dst, src.data(), std::min(SIZE, src.size()));
        dst[std::min(src.size(), SIZE - 1)] = 0;
    }

    class byte_view
    {
    public:
        byte_view(const std::byte* data, std::size_t data_size)
            : _data{ data }
            , _data_size{ data_size }
        { }

        byte_view(const byte_view&) = default;
        byte_view& operator=(const byte_view&) = default;


        auto data() const
        {
            return _data;
        }

        auto size() const
        {
            return _data_size;
        }

        std::unique_ptr<std::byte[]> clone() const
        {
            auto copyed_data = new std::byte[_data_size];
            std::memcpy(copyed_data, _data, _data_size);
            return std::unique_ptr<std::byte[]>(copyed_data);
        }

    private:
        const std::byte* _data = nullptr;
        std::size_t _data_size = 0;
    };
}