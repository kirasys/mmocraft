#pragma once
#include <utility>
#include <type_traits>

#include "noncopyable.h"

namespace util
{
    template<typename T>
    class DeferredCall : util::NonCopyable, util::NonMovable
    {
    public:
        DeferredCall(T&& callable)
            : _callable(std::forward<T>(callable))
        {
            static_assert(std::is_invocable_v<T>);
        }

        ~DeferredCall()
        {
            _callable();
        }
        
    private:
        T _callable;
    };

    template<typename T>
    using defer = DeferredCall<T>;
}

