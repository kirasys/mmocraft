#include "pch.h"
#include "uuid_v4.h"

namespace
{
    UUIDv4::UUIDGenerator<std::mt19937_64> uuid_generator;
}

namespace util {
    std::string uuid()
    {
        return uuid_generator.getUUID().str();
    }
}