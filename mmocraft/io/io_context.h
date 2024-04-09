#pragma once

#include <cstdint>
#include <memory>

#include <winsock2.h>
#include <mswsock.h>

#include "config/config.h"
#include "logging/error.h"
#include "win/win_type.h"
#include "win/smart_handle.h"

namespace io
{
	//constexpr int SEND_SMALL_BUF_SIZE  = 128;
	//constexpr int SEND_MEDIUM_BUF_SIZE = SEND_SMALL_BUF_SIZE * 8;
	//constexpr int SEND_LARGE_BUF_SIZE  = SEND_MEDIUM_BUF_SIZE * 8;

	constexpr int SEND_BUF_SIZE = 4096;
	constexpr int RECV_BUF_SIZE = 4096;

	struct IoContext
	{
		using handler_type = void (*)(void*, io::IoContext*, DWORD, DWORD);

		WSAOVERLAPPED overlapped;
		handler_type handler = nullptr;

		IoContext(handler_type handler_)
			: handler(handler_)
		{ }

		union Details
		{
			struct AcceptContext {
				LPFN_ACCEPTEX fnAcceptEx;
				win::Socket accepted_socket;
				std::uint8_t buffer[1024];
			} accept;

			struct SendContext {
				std::uint8_t buffer[SEND_BUF_SIZE];
			} send;

			struct RecvContext {
				std::uint8_t buffer[RECV_BUF_SIZE];
			} recv;

		} details;
	};
}