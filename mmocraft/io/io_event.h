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
#include "win/registered_io.h"
#include "config/constants.h"

namespace io
{
    namespace iocp_signal
    {
        constexpr DWORD eof = 0;
        constexpr DWORD event_failed = std::numeric_limits<DWORD>::max();
        constexpr DWORD retry_packet_process = event_failed - 1;
        constexpr DWORD task_completed = event_failed - 2;
    }

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

        virtual std::size_t capacity() const = 0;

        // buffer points to free space.

        virtual std::byte* begin_unused() = 0;

        virtual std::byte* end_unused() = 0;

        virtual std::size_t unused_size() const = 0;

        virtual bool push(const std::byte* data, std::size_t n) = 0;

        virtual void pop(std::size_t n) = 0;
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

        std::size_t capacity() const
        {
            return _capacity;
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
        std::byte buffer[config::memory::tcp_recv_buffer_size];
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

        std::size_t capacity() const
        {
            return _capacity;
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
        std::byte buffer[config::memory::tcp_send_buffer_size];
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

        std::size_t capacity() const
        {
            return _capacity;
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
        std::byte buffer[config::memory::tcp_send_buffer_size];
    };

    class IoSendEventLockFreeRawData : public IoSendEventLockFreeDataImpl
    {
    public:
        IoSendEventLockFreeRawData(void* buf, std::size_t buf_size)
            : IoSendEventLockFreeDataImpl{ buf, buf_size }
        { }
    };

    class IoSendEventReadonlyData : public IoEventData
    {
    public:
        IoSendEventReadonlyData() = default;

        IoSendEventReadonlyData(std::byte* data, std::size_t data_size)
            : _data{ data }
            , _data_size{ data_size }
        { }

        // data points to used space.

        std::byte* begin()
        {
            return _data;
        }

        std::byte* end()
        {
            return _data + _data_size;
        }

        std::size_t size() const
        {
            return _data_size;
        }

        std::size_t capacity() const
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
            return end();
        }

        std::size_t unused_size() const
        {
            return 0;
        }

        bool push(const std::byte* data, std::size_t n) override
        {
            return false;
        }

        void pop(std::size_t) override
        {
            // TODO: handling partial send;
            return;
        }

        void set_data(std::byte* data, std::size_t data_size)
        {
            _data = data;
            _data_size = data_size;
        }

    private:
        std::byte* _data = nullptr;
        std::size_t _data_size = 0;
    };

    class IoMulticastEventData : util::NonCopyable
    {
    public:
        IoMulticastEventData(std::unique_ptr<std::byte[]>&& data, std::size_t data_size)
            : _data{ std::move(data) }
            , _data_size{ data_size }
            , registered_buffer{ _data.get(), data_size }
        {
            
        }

        ~IoMulticastEventData()
        {
        
        }

        std::byte* begin()
        {
            return _data.get();
        }

        std::size_t size() const
        {
            return _data_size;
        }

        RIO_BUFFERID registered_buffer_id() const
        {
            return registered_buffer.id();
        }

    private:
        std::unique_ptr<std::byte[]> _data;
        std::byte* _data_temp;
        std::size_t _data_size = 0;

        win::RioBufferPool registered_buffer;
    };

    struct Event : util::NonCopyable
    {
        WSAOVERLAPPED overlapped = {};

        virtual void on_event_complete(IoEventHandler* completion_key, DWORD transferred_bytes) = 0;
    };

    struct IoEvent : Event
    {
        bool is_processing = false;

        IoEvent(IoEventData* a_data = nullptr)
            : _event_data{ a_data }
        { }

        ~IoEvent()
        {
            reset();
        }

        void reset(IoEventData* other = nullptr)
        {
            is_processing = false;

            if (is_non_owning_event_data)
                _event_data.release();

            _event_data.reset(other);
        }

        io::IoEventData* event_data()
        {
            return _event_data.get();
        }

        void set_non_owning_event_data()
        {
            is_non_owning_event_data = true;
        }

    private:
        bool is_non_owning_event_data = false;

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

        virtual void on_event_complete(IoEventHandler* completion_key, DWORD transferred_bytes) override;
    };

    struct IoConnectEvent : IoEvent
    {
        win::Socket connected_socket;

        IoConnectEvent(IoEventData* data = nullptr)
            : IoEvent{ data ? data : new io::IoRecvEventData() }
        { }

        bool post_overlapped_io(win::Socket, std::string_view ip, int port);

        virtual void on_event_complete(IoEventHandler* completion_key, DWORD transferred_bytes) override;
    };

    struct IoRecvEvent : IoEvent
    {
        using IoEvent::IoEvent;

        virtual void on_event_complete(IoEventHandler* completion_key, DWORD transferred_bytes) override;

        bool post_rio_event(RegisteredIO&, unsigned connection_id);
    };

    struct IoSendEvent : IoEvent
    {
        using IoEvent::IoEvent;

        bool post_overlapped_io(win::Socket);

        bool post_rio_event(RegisteredIO&, unsigned connection_id);

        virtual void on_event_complete(IoEventHandler* completion_key, DWORD transferred_bytes) override;
    };

    struct IoMulticastSendEvent : IoEvent
    {
        IoMulticastSendEvent()
            : IoEvent{ &_event_data }
        {
            IoEvent::set_non_owning_event_data();
        }

        ~IoMulticastSendEvent()
        {
            
        }

        bool post_rio_event(RegisteredIO&, unsigned connection_id);

        virtual void on_event_complete(IoEventHandler* completion_key, DWORD transferred_bytes) override;

        void set_multicast_data(std::shared_ptr<io::IoMulticastEventData>& data)
        {
            _event_data.set_data(data->begin(), data->size());
            multicast_data = data;
        }

    private:
        io::IoSendEventReadonlyData _event_data;

        std::shared_ptr<io::IoMulticastEventData> multicast_data;
    };

    struct RioEvent : IoEvent
    {
        using IoEvent::IoEvent;

        virtual void on_event_complete(IoEventHandler* completion_key, DWORD transferred_bytes) override;
    };

    class IoEventHandler
    {
    public:
        virtual void on_error() = 0;

        virtual void on_complete(IoAcceptEvent*) { assert(false); }

        virtual void on_complete(IoConnectEvent*) { assert(false); }

        virtual void on_complete(IoRecvEvent*) { assert(false); }

        virtual void on_complete(IoSendEvent*, std::size_t) { assert(false); }

        virtual void on_complete(IoMulticastSendEvent*, std::size_t) { assert(false); }

        virtual std::size_t handle_io_event(IoSendEvent*)
        {
            assert(false); return 0;
        }

        virtual std::size_t handle_io_event(IoAcceptEvent*)
        {
            assert(false); return 0;
        }

        virtual std::size_t handle_io_event(IoConnectEvent*)
        {
            assert(false); return 0;
        }

        virtual std::size_t handle_io_event(IoRecvEvent*)
        {
            assert(false); return 0;
        }
    };
}