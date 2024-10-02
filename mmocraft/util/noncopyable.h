#pragma once
namespace util
{
    struct NonCopyable
    {
        NonCopyable() = default;
        NonCopyable(NonCopyable&) = delete;
        NonCopyable& operator=(NonCopyable&) = delete;
    };

    struct NonMovable
    {
        NonMovable() = default;
        NonMovable(NonMovable const&) = delete;
        NonMovable& operator=(NonMovable const&) = delete;
    };
}