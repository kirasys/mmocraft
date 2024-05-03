#include "pch.h"
#include "io_event.h"

namespace io
{
	void IoAcceptEvent::invoke_handler(IoEventHandler& event_handler, DWORD transferred_bytes)
	{
		if (event_handler.handle_io_event(event_type, this).has_value())
			event_handler.on_success();
		else
			event_handler.on_error();
	}

	void IoRecvEvent::invoke_handler(IoEventHandler& event_handler, DWORD transferred_bytes)
	{
		// pre-processing
		if (transferred_bytes == 0)	// EOF
			return event_handler.on_error();

		data.push(nullptr, transferred_bytes); // data was already appended by I/O.

		// deliver events to the owner.
		auto processed_bytes = event_handler.handle_io_event(event_type, this);

		// post-processing
		if (not processed_bytes.has_value())
			return event_handler.on_error();

		data.pop(processed_bytes.value());

		event_handler.on_success();
	}

	bool IoRecvEventData::push(std::uint8_t*, std::size_t n)
	{
		// data was already appended by I/O.
		_size += n;
		return true;
	}

	void IoRecvEventData::pop(std::size_t n)
	{
		if (_size -= n)
			std::memmove(_data, _data + n, _size); // move remaining data ahead.
	}

	void IoSendEvent::invoke_handler(IoEventHandler& event_handler, DWORD transferred_bytes)
	{
		// pre-processing
		if (transferred_bytes == 0)	// EOF
			return event_handler.on_error();

		data.pop(transferred_bytes);

		// deliver events to the owner.
		// auto processed_bytes = event_handler.handle_io_event(event_type, this);
	}
}