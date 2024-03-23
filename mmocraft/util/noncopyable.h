#pragma once
namespace util
{
    struct NonCopyable
    {
        NonCopyable() = default;
        NonCopyable(NonCopyable&&) = default;
        NonCopyable& operator=(NonCopyable&&) = default;
    };

    struct NonMovable
    {
        NonMovable() = default;
        NonMovable(NonMovable const&) = delete;
        NonMovable& operator=(NonMovable const&) = delete;
    };
}