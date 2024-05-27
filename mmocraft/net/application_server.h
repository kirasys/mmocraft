#pragma once

#include "net/packet.h"
#include "net/connection_descriptor.h"

namespace net
{
	class ApplicationServer
	{
	public:
		virtual bool handle_packet(ConnectionLevelDescriptor, Packet*) = 0;
	};
}