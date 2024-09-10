#pragma once

#include <cstring>
#include <cstdint>
#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <limits>

#include "logging/error.h"
#include "win/win_type.h"
#include "win/smart_handle.h"

namespace io
{
    constexpr DWORD EOF_SIGNAL = 0;
    constexpr DWORD RETRY_SIGNAL = std::numeric_limits<DWORD>::max();
    constexpr DWORD IO_TASK_SIGNAL = RETRY_SIGNAL - 1;

    constexpr int RECV_BUFFER_SIZE = 4096;
    constexpr int SEND_BUFFER_SIZE = 8192;
    constexpr int CONCURRENT_SEND_BUFFER_SIZE = 4096;
    constexpr int SEND_SMALL_BUFFER_SIZE = 256;

    using IoEventResult = OVERLAPPED_ENTRY;

    using RioEventResult = RIORESULT;

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

        // 
    };

    class IoRecvEventDataImpl : public IoEventData
    {
    public:

        IoRecvEventDataImpl(void* buf, std::size_t buf_size)
            : _data{ reinterpret_cast<std::byte*>(buf) }
            , _capacity{ unsigned(buf_size) }
        { }

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
            return _data + _capacity;
        }

        std::size_t unused_size() const
        {
            return _capacity - _size;
        }

        bool push(const std::byte*, std::size_t n) override;

        void pop(std::size_t n) override;

    private:
        std::byte* _data = nullptr;
        unsigned _size = 0;
        unsigned _capacity = 0;
    };

    class IoRecvEventData : public IoRecvEventDataImpl
    {
    public:
        IoRecvEventData()
            : IoRecvEventDataImpl{ buffer, sizeof(buffer) }
        { }

    private:
        std::byte buffer[RECV_BUFFER_SIZE];
    };

    class IoRecvEventRawData : public IoRecvEventDataImpl
    {
    public:
        IoRecvEventRawData(void* buffer, std::size_t buffer_size)
            : IoRecvEventDataImpl{ buffer, buffer_size }
        { }
    };

    class IoRecvEventDynData : public IoRecvEventDataImpl
    {
    public:
        IoRecvEventDynData(std::byte* buffer, std::size_t buffer_size)
            : _buffer{ buffer }
            , IoRecvEventDataImpl{ buffer, buffer_size }
        { }

    private:
        std::unique_ptr<std::byte[]> _buffer;
    };

    using IoAcceptEventData = IoRecvEventData;

    class IoSendEventDataImpl : public IoEventData
    {
    public:
        IoSendEventDataImpl(void* buf, std::size_t buf_size)
            : _data{ reinterpret_cast<std::byte*>(buf) }
            , _capacity{ buf_size }
        { }

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
            return _data + _capacity;
        }

        std::size_t unused_size() const
        {
            return _capacity - data_tail;
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
            assert(data_head <= _capacity);
        }

    private:
        std::byte* _data = nullptr;
        int data_head = 0;
        int data_tail = 0;
        std::size_t _capacity = 0;
    };

    class IoSendEventData : public IoSendEventDataImpl
    {
    public:
        IoSendEventData()
            : IoSendEventDataImpl{ buffer, sizeof(buffer) }
        { }

    private:
        std::byte buffer[SEND_BUFFER_SIZE];
    };

    class IoSendEventRawData : public IoSendEventDataImpl
    {
    public:
        IoSendEventRawData(void* buf, std::size_t buf_size)
            : IoSendEventDataImpl{ buf, buf_size }
        { }
    };

    class IoSendEventDynData : public IoSendEventDataImpl
    {
    public:
        IoSendEventDynData(std::byte* buffer, std::size_t buffer_size)
            : _buffer{ buffer }
            , IoSendEventDataImpl{ buffer, buffer_size }
        { }

    private:
        std::unique_ptr<std::byte[]> _buffer;
    };

    class IoSendEventLockFreeDataImpl : public IoEventData
    {
    public:

        IoSendEventLockFreeDataImpl(void* buf, std::size_t buf_size)
            : _data{ reinterpret_cast<std::byte*>(buf) }
            , _capacity{ buf_size }
        {

        }

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
            return _data + _capacity;
        }

        std::size_t unused_size() const
        {
            return _capacity - data_tail.load(std::memory_order_relaxed);
        }

        bool push(const std::byte* data, std::size_t n) override
        {
            auto old_tail = data_tail.load(std::memory_order_relaxed);
            do {
                if (old_tail + int(n) > _capacity)
                    return false;
            } while (not data_tail.compare_exchange_weak(old_tail, old_tail + int(n),
                std::memory_order_relaxed, std::memory_order_relaxed));

            std::memcpy(_data + old_tail, data, n);
            return true;
        }

        void pop(std::size_t n) override
        {
            data_head += int(n);
            assert(data_head <= data_tail);

            auto tmp_data_head = data_head;
            if (data_tail.compare_exchange_strong(tmp_data_head, 0,
                std::memory_order_release, std::memory_order_relaxed))
                data_head = 0;
        }

    private:
        std::byte* _data = nullptr;
        int data_head = 0;
        std::atomic<int> data_tail{ 0 };

        std::size_t _capacity = 0;
    };

    class IoSendEventLockFreeData : public IoSendEventLockFreeDataImpl
    {
    public:
        IoSendEventLockFreeData()
            : IoSendEventLockFreeDataImpl{ buffer, sizeof(buffer) }
        { }

    private:
        std::byte buffer[CONCURRENT_SEND_BUFFER_SIZE];
    };

    class IoSendEventLockFreeRawData : public IoSendEventLockFreeDataImpl
    {
    public:
        IoSendEventLockFreeRawData(void* buf, std::size_t buf_size)
            : IoSendEventLockFreeDataImpl{ buf, buf_size }
        { }
    };

    class IoSendEventSharedData : public IoEventData
    {
    public:
        IoSendEventSharedData(std::unique_ptr<std::byte[]>&& data, unsigned data_size, unsigned data_capacity)
            : _data{ std::move(data) }
            , _data_size{ data_size }
            , _data_capacity{ data_capacity }
            , lifetime_end_at{ max_data_lifetime + util::current_monotonic_tick() }
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
        unsigned _data_size = 0;
        unsigned _data_capacity = 0;

        static constexpr std::size_t max_data_lifetime = 5 * 1000; // 5 seconds.
        std::size_t lifetime_end_at = 0;
    };

    enum class EventType
    {
        NONE,

        ACCPET,

        RECV,

        SEND,
    };

    struct Event : util::NonCopyable
    {
        WSAOVERLAPPED overlapped = {};

        virtual void on_event_complete(void* completion_key, DWORD transferred_bytes) = 0;
    };

    struct IoEvent : Event
    {
        bool is_processing = false;

        IoEvent(IoEventData* a_data = nullptr)
            : _event_data{ a_data }
        { }

        virtual ~IoEvent() = default;

        void set_event_data(IoEventData* a_data)
        {
            _event_data.reset(a_data);
        }

        io::IoEventData* event_data()
        {
            return _event_data.get();
        }

        void set_disposable()
        {
            _is_disposable = true;
        }

        bool is_disposable() const
        {
            return _is_disposable;
        }

    private:
        bool _is_disposable = false;

        std::unique_ptr<IoEventData> _event_data;
    };

    class RegisteredIO;

    struct IoAcceptEvent : IoEvent
    {
        win::Socket accepted_socket;

        IoAcceptEvent(IoEventData* data = nullptr)
            : IoEvent{ data ? data : new io::IoRecvEventData() }
        { }

        bool post_overlapped_io(win::Socket);

        void on_event_complete(void* completion_key, DWORD transferred_bytes) override;
    };

    struct IoRecvEvent : IoEvent
    {
        using IoEvent::IoEvent;

        void on_event_complete(void* completion_key, DWORD transferred_bytes) override;

        bool post_rio_event(RegisteredIO&, unsigned connection_id);
    };

    struct IoSendEvent : IoEvent
    {
        using IoEvent::IoEvent;

        bool post_overlapped_io(win::Socket);

        bool post_rio_event(RegisteredIO&, unsigned connection_id);

        void on_event_complete(void* completion_key, DWORD transferred_bytes) override;

        static IoSendEvent* create_disposable_event(std::size_t data_size)
        {
            auto io_event = new io::IoSendEvent(
                new io::IoSendEventDynData(new std::byte[data_size], data_size)
            );

            io_event->set_disposable();

            return io_event;
        }
    };

    struct RioEvent : IoEvent
    {
        using IoEvent::IoEvent;

        void on_event_complete(void* completion_key, DWORD transferred_bytes) override;
    };

    class IoEventHandler
    {
    public:
        virtual void on_error() = 0;

        virtual void on_complete(IoAcceptEvent*) { assert(false); }

        virtual void on_complete(IoRecvEvent*) { assert(false); }

        virtual void on_complete(IoSendEvent*, std::size_t) { assert(false); }

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