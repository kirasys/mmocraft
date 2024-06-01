#pragma once

#include "net/packet.h"
#include "net/connection_descriptor.h"

#include "logging/error.h"

namespace net
{
	class ApplicationServer
	{
	public:
		virtual error::ErrorCode handle_packet(net::DescriptorType::Connection, net::Packet*) = 0;
	};
}