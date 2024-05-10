#pragma once

#include <cstring>
#include <cstdint>
#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>

#include <winsock2.h>
#include <mswsock.h>

#include "config/config.h"
#include "logging/error.h"
#include "win/win_type.h"
#include "win/smart_handle.h"

namespace io
{

	constexpr int RECV_BUFFER_SIZE = 4096;
	constexpr int SEND_BUFFER_SIZE = 4096;

	enum EventType
	{
		Invalid,
		AcceptEvent,
		RecvEvent,
		SendEvent,
	};

	class IoEventData;
	class IoEventHandler;

	struct IoEvent
	{
		WSAOVERLAPPED overlapped;
		
		// NOTE: separate buffer space for better locality.
		//       the allocator(may be pool) responsible for release.
		IoEventData& data;

		IoEvent(IoEventData* a_data)
			: data{*a_data }
		{ }

		virtual void invoke_handler(IoEventHandler&, DWORD) = 0;
	};

	struct IoAcceptEvent : IoEvent
	{
		static const EventType event_type = AcceptEvent;

		LPFN_ACCEPTEX fnAcceptEx;
		win::Socket accepted_socket;

		using IoEvent::IoEvent;

		void invoke_handler(IoEventHandler& event_handler, DWORD transferred_bytes) override;
	};

	struct IoRecvEvent : IoEvent
	{
		static const EventType event_type = RecvEvent;

		using IoEvent::IoEvent;

		void invoke_handler(IoEventHandler& event_handler, DWORD transferred_bytes) override;
	};

	struct IoSendEvent : IoEvent
	{
		static const EventType event_type = SendEvent;

		using IoEvent::IoEvent;

		void invoke_handler(IoEventHandler&, DWORD) override;
	};

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

	class IoSendEventData : public IoEventData
	{
		// data points to used space.

		std::byte* begin()
		{
			return _data + used_data_head;
		}

		std::byte* end()
		{
			return _data + used_data_tail;
		}

		std::size_t size() const
		{
			return used_data_tail - used_data_head;
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
			return sizeof(_data) - used_data_tail;
		}

		bool push(std::byte* data, std::size_t n) override;

		void pop(std::size_t n) override;

	private:
		std::byte _data[SEND_BUFFER_SIZE];
		std::size_t used_data_head = 0;
		std::size_t used_data_tail = 0;
	};
	
	class IoEventHandler
	{
	public:
		virtual void on_error(IoEvent*) = 0;

		virtual void on_success(IoEvent*) = 0;

		virtual std::optional<std::size_t> handle_io_event(EventType, IoEvent*) = 0;
	};
}