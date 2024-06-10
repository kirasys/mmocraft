#pragma once
#include <utility>
#include <string_view>

namespace util
{
    bool gzip_compress(std::string_view source, std::string& dest);
}