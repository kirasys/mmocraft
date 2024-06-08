#pragma once

#include <gmock/gmock.h>
#include "net/socket.h"

namespace test
{
    class MockSocket : public net::Socket
    {
        using Socket::Socket;

        MOCK_METHOD(error::ErrorCode, accept, (io::IoAcceptEvent&), ());
    };
}