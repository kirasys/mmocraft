#pragma once

#include <cstring>
#include <cstdint>
#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <limits>

#define NOMINMAX
#include <winsock2.h>
#include <mswsock.h>

#include "config/config.h"
#include "logging/error.h"
#include "win/win_type.h"
#include "win/smart_handle.h"

namespace io
{
    constexpr DWORD EOF_SIGNAL			= 0;
    constexpr DWORD RETRY_SIGNAL		= std::numeric_limits<DWORD>::max();
    constexpr DWORD IO_TASK_SIGNAL = RETRY_SIGNAL - 1;

    constexpr int RECV_BUFFER_SIZE = 4096;
    constexpr int SEND_BUFFER_SIZE = 1024;
    constexpr int CONCURRENT_SEND_BUFFER_SIZE = 2048;
    constexpr int SEND_SMALL_BUFFER_SIZE = 256;

    class IoEventHandler;

    class IoEventData
    {
    public:
        virtual ~IoEventData() = default;

        // data points to used space.

        virtual std::byte* begin() = 0;

        virtual std::byte* end() = 0;

        virtual std::size_t size() const = 0;

        // buffer points to free space.

        virtual std::byte* begin_unused() = 0;

        virtual std::byte* end_unused() = 0;

        virtual std::size_t unused_size() const = 0;
        
        virtual bool push(const std::byte* data, std::size_t n) = 0;

        virtual void pop(std::size_t n) = 0;
    };

    class IoRecvEventData : public IoEventData
    {
    public:
        // data points to used space.

        std::byte* begin()
        {
            return _data;
        }

        std::byte* end()
        {
            return _data + _size;
        }

        std::size_t size() const
        {
            return _size;
        }

        // buffer points to free space.

        std::byte* begin_unused()
        {
            return end();
        }

        std::byte* end_unused()
        {
            return _data + sizeof(_data);
        }

        std::size_t unused_size() const
        {
            return sizeof(_data) - _size;
        }

        bool push(const std::byte*, std::size_t n) override;

        void pop(std::size_t n) override;

    private:
        std::byte _data[RECV_BUFFER_SIZE] = {};
        std::size_t _size = 0;
    };

    using IoAcceptEventData = IoRecvEventData;

    template <std::size_t N>
    class IoSendEventVariableData : public IoEventData
    {
    public:
        // data points to used space.

        std::byte* begin()
        {
            return _data + data_head;
        }

        std::byte* end()
        {
            return _data + data_tail;
        }

        std::size_t size() const
        {
            return std::size_t(data_tail - data_head);
        }

        // buffer points to free space.

        std::byte* begin_unused()
        {
            return end();
        }

        std::byte* end_unused()
        {
            return _data + sizeof(_data);
        }

        std::size_t unused_size() const
        {
            return sizeof(_data) - data_tail;
        }

        bool push(const std::byte* data, std::size_t n) override
        {
            if (data_head == data_tail)
                data_head = data_tail = 0;

            if (n > unused_size())
                return false;

            std::memcpy(begin_unused(), data, n);
            data_tail += int(n);

            return true;
        }

        void pop(std::size_t n) override
        {
            data_head += int(n);
            assert(data_head <= sizeof(_data));
        }

    private:
        std::byte _data[N] = {};
        int data_head = 0;
        int data_tail = 0;
    };

    using IoSendEventData = IoSendEventVariableData<SEND_BUFFER_SIZE>;
    using IoSendEventSmallData = IoSendEventVariableData<SEND_SMALL_BUFFER_SIZE>;

    class IoSendEventLockFreeData : public IoEventData
    {
    public:
        // data points to used space.

        std::byte* begin()
        {
            return _data + data_head;
        }

        std::byte* end()
        {
            return _data + data_tail.load(std::memory_order_relaxed);
        }

        std::size_t size() const 
        {
            return std::size_t(data_tail.load(std::memory_order_relaxed) - data_head);
        }

        // buffer points to free space.

        std::byte* begin_unused()
        {
            return end();
        }

        std::byte* end_unused()
        {
            return _data + sizeof(_data);
        }

        std::size_t unused_size() const
        {
            return sizeof(_data) - data_tail.load(std::memory_order_relaxed);
        }

        bool push(const std::byte* data, std::size_t n) override
        {
            auto old_tail = data_tail.load(std::memory_order_relaxed);
            do {
                if (old_tail + int(n) > sizeof(_data))
                    return false;
            } while (not data_tail.compare_exchange_weak(old_tail, old_tail + int(n),
                    std::memory_order_relaxed, std::memory_order_relaxed));

            std::memcpy(_data + old_tail, data, n);
            return true;
        }

        void pop(std::size_t n) override
        {
            data_head += int(n);
            assert(data_head <= sizeof(_data));

            auto tmp_data_head = data_head;
            if (data_tail.compare_exchange_strong(tmp_data_head, 0,
                std::memory_order_release, std::memory_order_relaxed))
                data_head = 0;
        }

    private:
        std::byte _data[CONCURRENT_SEND_BUFFER_SIZE];
        int data_head;
        std::atomic<int> data_tail;
    };

    class IoSendEventSharedData : public IoEventData
    {
    public:
        IoSendEventSharedData(std::unique_ptr<std::byte[]>&& data, unsigned data_size, unsigned data_capacity)
            : _data{ std::move(data) }
            , _data_size{ data_size }
            , _data_capacity{ data_capacity }
            , lifetime_end_at{max_data_lifetime + util::current_monotonic_tick()}
        { }

        // data points to used space.

        std::byte* begin()
        {
            return _data.get();
        }

        std::byte* end()
        {
            return _data.get() + _data_size;
        }

        std::size_t size() const
        {
            return _data_size;
        }

        // buffer points to free space.

        std::byte* begin_unused()
        {
            return end();
        }

        std::byte* end_unused()
        {
            return _data.get() + _data_capacity;
        }

        std::size_t unused_size() const
        {
            return _data_capacity - _data_size;
        }

        bool push(const std::byte* data, std::size_t n) override
        {
            if (n > unused_size())
                return false;

            std::memcpy(begin_unused(), data, n);
            _data_size += unsigned(n);

            return true;
        }

        void pop(std::size_t) override
        {
            // TODO: handling partial send;
        }

        void update_lifetime()
        {
            lifetime_end_at = max_data_lifetime + util::current_monotonic_tick();
        }

        bool is_safe_delete() const
        {
            return lifetime_end_at < util::current_monotonic_tick();
        }

    private:
        std::unique_ptr<std::byte[]> _data;
        unsigned _data_size;
        unsigned _data_capacity;

        static constexpr std::size_t max_data_lifetime = 5 * 1000; // 5 seconds.
        std::size_t lifetime_end_at;
    };

    struct IoEvent
    {
        WSAOVERLAPPED overlapped = {};

        bool is_processing = false;

        // NOTE: separate buffer space for better locality.
        //       the allocator(may be pool) responsible for release.
        IoEventData* data;

        IoEvent(IoEventData* a_data)
            : data{ a_data }
        { }

        virtual ~IoEvent() = default;

        void set_data(IoEventData* a_data)
        {
            data = a_data;
        }

        virtual void invoke_handler(IoEventHandler&, DWORD transferred_bytes) = 0;
    };

    struct IoAcceptEvent : IoEvent
    {
        LPFN_ACCEPTEX fnAcceptEx;
        win::Socket accepted_socket;

        using IoEvent::IoEvent;

        void invoke_handler(IoEventHandler&, DWORD) override;
    };

    struct IoRecvEvent : IoEvent
    {
        using IoEvent::IoEvent;

        void invoke_handler(IoEventHandler&, DWORD) override;
    };

    struct IoSendEvent : IoEvent
    {
        using IoEvent::IoEvent;

        void invoke_handler(IoEventHandler&, DWORD) override;
    };
    
    class IoEventHandler
    {
    public:
        virtual void on_error() = 0;

        virtual void on_complete(IoAcceptEvent*) { assert(false); }

        virtual void on_complete(IoRecvEvent*) { assert(false); }

        virtual void on_complete(IoSendEvent*) { assert(false); }

        virtual std::size_t handle_io_event(IoAcceptEvent*)
        {
            assert(false); return 0;
        }

        virtual std::size_t handle_io_event(IoRecvEvent*)
        {
            assert(false); return 0;
        }

        virtual std::size_t handle_io_event(IoSendEvent*)
        {
            assert(false); return 0;
        }
    };
}