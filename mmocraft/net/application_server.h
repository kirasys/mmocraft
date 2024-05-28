#pragma once

#include "net/packet.h"
#include "net/connection_descriptor.h"

#include "logging/error.h"

namespace net
{
	class ApplicationServer
	{
	public:
		virtual bool handle_packet(ConnectionLevelDescriptor, net::Packet*, error::ErrorCode) = 0;
	};
}