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

		data.size += transferred_bytes;

		// deliver events to the owner.
		auto processed_bytes = event_handler.handle_io_event(event_type, this);

		// post-processing
		if (not processed_bytes.has_value())
			return event_handler.on_error();

		if (data.size - processed_bytes.value())
			std::memmove(data.begin(), data.begin() + processed_bytes.value(), data.size);

		event_handler.on_success();
	}
}