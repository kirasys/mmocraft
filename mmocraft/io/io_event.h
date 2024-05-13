#pragma once

#include <cstring>
#include <cstdint>
#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <limits>

#include <winsock2.h>
#include <mswsock.h>

#include "config/config.h"
#include "logging/error.h"
#include "win/win_type.h"
#include "win/smart_handle.h"

namespace io
{
	constexpr DWORD EOF_SIGNAL = 0;
	constexpr DWORD RETRY_SIGNAL = std::numeric_limits<DWORD>::max();

	constexpr int RECV_BUFFER_SIZE = 4096;
	constexpr int SEND_BUFFER_SIZE = 4096;
	constexpr int SEND_SMALL_BUFFER_SIZE = 1024;

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
		
		virtual bool push(std::byte* data, std::size_t n) = 0;

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

		bool push(std::byte*, std::size_t n) override;

		void pop(std::size_t n) override;

	private:
		std::byte _data[RECV_BUFFER_SIZE];
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

		bool push(std::byte* data, std::size_t n) override
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
		std::byte _data[N];
		int data_head = 0;
		int data_tail = 0;
	};

	using IoSendEventData = IoSendEventVariableData<SEND_BUFFER_SIZE>;
	using IoSendEventSmallData = IoSendEventVariableData<SEND_SMALL_BUFFER_SIZE>;

	struct IoEvent
	{
		WSAOVERLAPPED overlapped;

		// NOTE: separate buffer space for better locality.
		//       the allocator(may be pool) responsible for release.
		IoEventData& data;

		IoEvent(IoEventData* a_data)
			: data{ *a_data }
		{ }

		virtual void invoke_handler(IoEventHandler&, DWORD) = 0;
	};

	struct IoAcceptEvent : IoEvent
	{
		LPFN_ACCEPTEX fnAcceptEx;
		win::Socket accepted_socket;

		using IoEvent::IoEvent;

		void invoke_handler(IoEventHandler& event_handler, DWORD transferred_bytes) override;
	};

	struct IoRecvEvent : IoEvent
	{
		using IoEvent::IoEvent;

		void invoke_handler(IoEventHandler& event_handler, DWORD transferred_bytes) override;
	};

	struct IoSendEvent : IoEvent
	{
		std::size_t transferred_small_data_bytes = 0;
		IoSendEventSmallData& small_data;

		IoSendEvent(IoSendEventData* a_data, IoSendEventSmallData* a_small_data)
			: IoEvent{ a_data }
			, small_data{ *a_small_data }
		{ }

		void invoke_handler(IoEventHandler&, DWORD) override;
	};
	
	class IoEventHandler
	{
	public:
		virtual void on_error(IoAcceptEvent*) { assert(false); };

		virtual void on_error(IoRecvEvent*) { assert(false); };

		virtual void on_error(IoSendEvent*) { assert(false); };

		virtual void on_success(IoAcceptEvent*) { assert(false); };

		virtual void on_success(IoRecvEvent*) { assert(false); };

		virtual void on_success(IoSendEvent*) { assert(false); };

		virtual std::optional<std::size_t> handle_io_event(IoAcceptEvent*) { assert(false); return std::nullopt;};

		virtual std::optional<std::size_t> handle_io_event(IoRecvEvent*) { assert(false); return std::nullopt;};

		virtual std::optional<std::size_t> handle_io_event(IoSendEvent*) { assert(false); return std::nullopt;};
	};
}