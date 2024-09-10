#pragma once

#define NOMINMAX
#include <winsock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#include <Windows.h>

namespace win
{
    using Handle = HANDLE;
    using Socket = SOCKET;
}