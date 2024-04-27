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

	constexpr int SEND_BUF_SIZE = 4096;
	constexpr int RECV_BUF_SIZE = 4096;

	enum EventType
	{
		Invalid,
		AcceptEvent,
		RecvEvent,
		SendEvent,
	};

	struct IoEvent;

	class IoEventHandler
	{
	public:
		virtual void on_error() = 0;

		virtual void on_success() = 0;

		virtual std::optional<std::size_t> handle_io_event(EventType) = 0;
	};

	struct IoEvent
	{
		WSAOVERLAPPED overlapped;

		IoEvent() = default;

		virtual void invoke_handler(IoEventHandler*, DWORD) { } // TODO: should be pure virtual function?
	};

	struct IoAcceptEvent : IoEvent
	{
		static const EventType event_type = AcceptEvent;

		LPFN_ACCEPTEX fnAcceptEx;
		win::Socket accepted_socket;

		std::uint8_t buffer[1024];

		void invoke_handler(IoEventHandler* event_handler, DWORD transferred_bytes) override
		{
			if (event_handler->handle_io_event(event_type).has_value())
				event_handler->on_success();
			else
				event_handler->on_error();
		}
	};

	struct IoRecvEvent : IoEvent
	{
		static const EventType event_type = RecvEvent;

		std::size_t buffer_size = 0;
		std::uint8_t buffer[RECV_BUF_SIZE];

		std::uint8_t* buffer_begin()
		{
			return buffer;
		}

		std::uint8_t* buffer_end()
		{
			return buffer + buffer_size;
		}

		std::uint8_t* unused_buffer_begin()
		{
			return buffer_end();
		}

		std::uint8_t* unused_buffer_end()
		{
			return buffer + sizeof(buffer);
		}
		
		void invoke_handler(IoEventHandler* event_handler, DWORD transferred_bytes) override
		{
			// pre-processing
			if (transferred_bytes == 0)	// EOF
				return event_handler->on_error();

			buffer_size += transferred_bytes;

			// deliver events to the owner.
			auto processed_bytes = event_handler->handle_io_event(event_type);

			// post-processing
			if (not processed_bytes.has_value())
				return event_handler->on_error();
			
			if (buffer_size -= processed_bytes.value())
				std::memmove(buffer, buffer + processed_bytes.value(), buffer_size);

			event_handler->on_success();
		}
	};

	struct IoSendEvent : IoEvent
	{
		static const EventType event_type = SendEvent;

		std::uint8_t buffer[SEND_BUF_SIZE];

		void invoke_handler(IoEventHandler*, DWORD) override
		{

		}
	};
}