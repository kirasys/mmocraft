#pragma once

#include "net/packet.h"

namespace net
{
	class ApplicationServer
	{
	public:
		virtual bool handle_packet(unsigned, Packet*) = 0;
	};
}