#pragma once

#include <memory>
#include <utility>
#include <type_traits>

#include "win/win_type.h"
#include "util/common_util.h"

namespace {
    void invalid_handle_deleter(win::Handle h) { }
}

namespace win
{
    class UniqueSocket : util::NonCopyable
    {
    public:
        UniqueSocket() noexcept
            : _handle{INVALID_SOCKET}
        { }

        UniqueSocket(win::Socket handle) noexcept
            : _handle(handle)
        { }

        ~UniqueSocket()
        { 
            clear();
        }

        UniqueSocket(UniqueSocket&& other) noexcept
            : _handle{ other._handle }
        {
            other._handle = INVALID_SOCKET;
        }

        UniqueSocket& operator=(UniqueSocket&& other) noexcept
        {
            if (_handle != other._handle) {
                clear();
                _handle = other._handle;
                other._handle = INVALID_SOCKET;
            }
            return *this;
        }

        operator win::Socket()
        {
            return _handle;
        }

        operator win::Socket() const
        {
            return _handle;
        }

        win::Socket get()
        {
            return _handle;
        }

        win::Socket get() const
        {
            return _handle;
        }
        
        void reset(win::Socket handle = INVALID_SOCKET)
        {
            clear();
            _handle = handle;
        }

        bool is_valid() const
        {
            return _handle != INVALID_SOCKET;
        }

    private:
        void clear()
        {
            if (is_valid())
                ::closesocket(_handle);
        }

        win::Socket _handle;
    };

    class UniqueHandle : util::NonCopyable
    {
    public:
        UniqueHandle() noexcept
            : _handle{ INVALID_HANDLE_VALUE }
        { }

        UniqueHandle(win::Handle handle) noexcept
            : _handle(handle)
        { }

        ~UniqueHandle()
        {
            clear();
        }

        bool is_valid() const
        {
            return _handle != INVALID_HANDLE_VALUE;
        }

        UniqueHandle(UniqueHandle&& other) noexcept
            : _handle{ other._handle }
        {
            other._handle = INVALID_HANDLE_VALUE;
        }

        UniqueHandle& operator=(UniqueHandle&& other) noexcept
        {
            if (_handle != other._handle) {
                clear();
                _handle = other._handle;
                other._handle = INVALID_HANDLE_VALUE;
            }
            return *this;
        }

        win::Handle get()
        {
            return _handle;
        }

        win::Handle get() const
        {
            return _handle;
        }

        void reset(win::Handle handle = INVALID_HANDLE_VALUE)
        {
            clear();
            _handle = handle;
        }

    private:
        void clear()
        {
            if (is_valid())
                ::CloseHandle(_handle);
        }

        win::Handle _handle;
    };

    class SharedHandle
    {
    public:
        SharedHandle()
            : _handle{ nullptr, invalid_handle_deleter }
        { }

        SharedHandle(win::Handle handle)
            : _handle{ handle, ::CloseHandle }
        { }

        // copy controllers
        SharedHandle(SharedHandle&) = default;
        SharedHandle& operator=(SharedHandle&) = default;

        // move controllers
        SharedHandle(SharedHandle&& handle) = default;
        SharedHandle& operator=(SharedHandle&& handle) = default;

        operator win::Handle()
        {
            return _handle.get();
        }

        operator win::Handle() const
        {
            return _handle.get();
        }

        win::Handle get() const
        {
            return _handle.get();
        }

        // reset operator
        void reset()
        {
            _handle.reset();
        }

    private:
        std::shared_ptr<std::remove_pointer_t<win::Handle>> _handle;
    };
}