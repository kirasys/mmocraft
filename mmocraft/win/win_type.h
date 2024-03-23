#pragma once

#define NOMINMAX
#include <winsock2.h>

namespace win
{
	using Handle = HANDLE;
	using Socket = SOCKET;
}