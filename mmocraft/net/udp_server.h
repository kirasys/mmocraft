#pragma once

#include <thread>

#include "net/socket.h"
#include "net/server_core.h"

namespace net
{
    constexpr int SOCKET_RCV_BUFFER_SIZE = 1024 * 1024 * 4; // 4MB
    constexpr int SOCKET_SND_BUFFER_SIZE = 1024 * 1024 * 4; // 4MB

    class UdpServer : public net::ServerCore
    {
    public:
        UdpServer(std::string_view ip, int port);

        ~UdpServer();

        void close();

        void start_network_io_service() override;

        std::thread spawn_event_loop_thread();

        void run_event_loop_forever(DWORD get_event_timeout_ms);

    private:
        std::string _ip;
        int _port;

        net::Socket _sock;

        bool is_terminated = false;
        std::vector<std::thread> event_threads;
    };
}