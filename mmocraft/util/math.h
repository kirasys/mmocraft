#pragma once

#include <cstdlib>

namespace util
{
    struct Coordinate3D
    {
        short x = 0;
        short y = 0;
        short z = 0;
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