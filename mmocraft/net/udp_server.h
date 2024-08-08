#pragma once

#include <array>
#include <cassert>
#include <thread>

#include "net/socket.h"
#include "net/server_core.h"
#include "net/udp_message.h"
#include "proto/generated/protocol.pb.h"

namespace net
{
    constexpr int SOCKET_RCV_BUFFER_SIZE = 1024 * 1024 * 4; // 4MB
    constexpr int SOCKET_SND_BUFFER_SIZE = 1024 * 1024 * 4; // 4MB
    
    class UdpServer : public net::ServerCore
    {
    public:
        UdpServer(MessageHandler&);

        ~UdpServer();

        void reset();

        bool send(const char* ip, int port, const char* data, std::size_t data_size);

        void start_network_io_service(std::string_view ip, int port, std::size_t num_of_event_threads) override;

        std::thread spawn_event_loop_thread();

        void run_event_loop_forever(DWORD get_event_timeout_ms);

    private:
        net::Socket _sock;
        MessageHandler& message_handler;

        bool is_terminated = false;
        std::vector<std::thread> event_threads;
    };
}