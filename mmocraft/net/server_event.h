#pragma once

namespace net
{
    enum ServerEventType
    {

    };

    class ServerEventHandler
    {
    public:
        virtual void handle_server_event(ServerEventType) = 0;
    };
}