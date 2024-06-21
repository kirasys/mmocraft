#pragma once

#include <cstdlib>

namespace util
{
    using Coordinate = short;

    struct Coordinate3D
    {
        Coordinate x = 0;
        Coordinate y = 0;
        Coordinate z = 0;
    };

    struct Orientation
    {
    public:
        static auto byte_to_degree(std::uint8_t packed)
        {
            return float(packed * 360.0f / 256.0f);
        }

        static auto degree_to_byte(float deg)
        {
            return std::uint8_t(deg * 256.0f / 360.0f);
        }
    };
}