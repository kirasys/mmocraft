#pragma once

#include <memory>

#include <winsock2.h>
#include <mswsock.h>

#include "config/config.h"
#include "logging/error.h"
#include "win/win_type.h"

namespace io
{
	constexpr int SEND_SMALL_BUF_SIZE  = 128;
	constexpr int SEND_MEDIUM_BUF_SIZE = SEND_SMALL_BUF_SIZE * 8;
	constexpr int SEND_LARGE_BUF_SIZE  = SEND_MEDIUM_BUF_SIZE * 8;

	constexpr int RECV_BUF_SIZE = 4096;
	using RecvBuffer = uint8_t[RECV_BUF_SIZE];
	
	struct IoContext
	{
		using handler_type = void (*)(void*, io::IoContext*, DWORD, DWORD);

		WSAOVERLAPPED overlapped;
		handler_type handler;
	};

	struct AcceptIoContext : IoContext
	{
		RecvBuffer buffer;
		LPFN_ACCEPTEX fnAcceptEx = nullptr;
		win::Socket accepted_socket = NULL;
	};

	struct RecvIoContext : IoContext
	{
		RecvBuffer buffer;
	};
}