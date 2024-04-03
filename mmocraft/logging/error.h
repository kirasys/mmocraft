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
			CREATE_SOCKET,
			BIND,
			LISTEN,
			ACCEPTEX_LOAD,
			ACCEPTEX_FAIL,
			SEND,
			RECV,
		};
	};

	std::ostream& operator<<(std::ostream& os, Exception::Network ex);
}