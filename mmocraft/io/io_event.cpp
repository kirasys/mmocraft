#include "pch.h"
#include "io_event.h"

namespace io
{
	void IoAcceptEvent::invoke_handler(IoEventHandler& event_handler, DWORD transferred_bytes)
	{
		if (event_handler.handle_io_event(this).has_value())
			event_handler.on_success(this);
		else
			event_handler.on_error(this);
	}

	void IoRecvEvent::invoke_handler(IoEventHandler& event_handler, DWORD transferred_bytes)
	{
		// pre-processing
		if (transferred_bytes == 0)	// EOF
			return event_handler.on_error(this);

		// deliver events to the owner.
		auto processed_bytes = event_handler.handle_io_event(this);

		// post-processing
		if (not processed_bytes.has_value())
			return event_handler.on_error(this);

		data.pop(processed_bytes.value());

		event_handler.on_success(this);
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

	void IoSendEvent::invoke_handler(IoEventHandler& event_handler, DWORD transferred_bytes)
	{
		// pre-processing
		if (transferred_bytes == 0)	// EOF
			return event_handler.on_error(this);

		auto processed_small_data_bytes = std::min(size_t(transferred_bytes), transferred_small_data_bytes);
		small_data.pop(processed_small_data_bytes);

		if (auto process_data_bytes = transferred_bytes - processed_small_data_bytes)
			data.pop(process_data_bytes);

		event_handler.on_success(this);
	}
}