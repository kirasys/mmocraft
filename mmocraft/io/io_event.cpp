#include "pch.h"
#include "io_event.h"

#include "util/deferred_call.h"

namespace io
{
	void IoAcceptEvent::invoke_handler(IoEventHandler& event_handler, DWORD transferred_bytes)
	{
		event_handler.handle_io_event(this);
		event_handler.on_complete(this);
	}

	void IoRecvEvent::invoke_handler(IoEventHandler& event_handler, DWORD transferred_bytes_or_signal)
	{
		// pre-processing
		if (transferred_bytes_or_signal == EOF_SIGNAL)	// EOF
			return;

		if (transferred_bytes_or_signal != RETRY_SIGNAL)
			data.push(nullptr, transferred_bytes_or_signal); // data was already appended by I/O. just update size only.

		// deliver events to the owner.
		auto processed_bytes = event_handler.handle_io_event(this);

		// post-processing
		if (processed_bytes)
			data.pop(processed_bytes);

		event_handler.on_complete(this);
	}

	bool IoRecvEventData::push(std::byte*, std::size_t n)
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

	void IoSendEvent::invoke_handler(IoEventHandler& event_handler, DWORD transferred_bytes_or_signal)
	{
		// pre-processing
		if (transferred_bytes_or_signal == EOF_SIGNAL)	// EOF
			return;

		data.pop(transferred_bytes_or_signal);

		event_handler.on_complete(this);
	}
}