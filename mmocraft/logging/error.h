#pragma once

#include <iostream>
#include <sstream>
#include <utility>
#include <source_location>

namespace error
{
	struct Exception
	{
		enum Network
		{
			SUCCESS = 0,			// success must be 0
			
			// Socket
			SOCKET_CREATE,
			SOCKET_BIND,
			SOCKET_LISTEN,
			SOCKET_ACCEPTEX_LOAD,
			SOCKET_ACCEPTEX,
			SOCKET_SEND,
			SOCKET_RECV,
		};
	};

	std::ostream& operator<<(std::ostream& os, Exception::Network ex);
}