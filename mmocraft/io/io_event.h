#pragma once

#include <cstring>
#include <cstdint>
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
	//constexpr int SEND_SMALL_BUF_SIZE  = 128;
	//constexpr int SEND_MEDIUM_BUF_SIZE = SEND_SMALL_BUF_SIZE * 8;
	//constexpr int SEND_LARGE_BUF_SIZE  = SEND_MEDIUM_BUF_SIZE * 8;

	constexpr int DEFAULT_BUFFER_SIZE = 4096;

	enum EventType
	{
		Invalid,
		AcceptEvent,
		RecvEvent,
		SendEvent,
	};

	class IoEventHandler
	{
	public:
		virtual void on_error() = 0;

		virtual void on_success() = 0;

		virtual std::optional<std::size_t> handle_io_event(EventType) = 0;
	};

	class IoEventData;

	struct IoEvent
	{
		WSAOVERLAPPED overlapped;
		
		// NOTE: separate buffer space for better locality.
		//       the allocator(may be pool) responsible for release.
		IoEventData& data;

		IoEvent(IoEventData* a_data)
			: data{*a_data }
		{ }

		virtual void invoke_handler(IoEventHandler*, DWORD) = 0;
	};

	struct IoAcceptEvent : IoEvent
	{
		static const EventType event_type = AcceptEvent;

		LPFN_ACCEPTEX fnAcceptEx;
		win::Socket accepted_socket;

		using IoEvent::IoEvent;

		void invoke_handler(IoEventHandler* event_handler, DWORD transferred_bytes) override;
	};

	struct IoRecvEvent : IoEvent
	{
		static const EventType event_type = RecvEvent;

		using IoEvent::IoEvent;

		void invoke_handler(IoEventHandler* event_handler, DWORD transferred_bytes) override;
	};

	struct IoSendEvent : IoEvent
	{
		static const EventType event_type = SendEvent;

		using IoEvent::IoEvent;

		void invoke_handler(IoEventHandler*, DWORD) override
		{

		}
	};

	class IoEventData
	{
	public:
		// data points to used space.

		std::uint8_t* begin()
		{
			return data;
		}

		std::uint8_t* end()
		{
			return data + size;
		}

		// buffer points to free space.

		std::uint8_t* begin_unused()
		{
			return end();
		}

		std::uint8_t* end_unused()
		{
			return data + sizeof(data);
		}

		std::size_t unused_size() const
		{
			return sizeof(data) - size;
		}

	private:
		// Only IoEvent class can access internals. (especially size)
		friend IoAcceptEvent;
		friend IoSendEvent;
		friend IoRecvEvent;
		std::uint8_t data[DEFAULT_BUFFER_SIZE];
		std::size_t size = 0;
	};
}