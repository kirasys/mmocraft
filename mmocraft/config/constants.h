#pragma once

namespace config
{
    // Compile time constants

    namespace task {
        constexpr int announce_server_period = 5000; // 5s
    }

    namespace network {
        constexpr int udp_message_retransmission_period = 3000; // 3s
    }

    namespace memory {
        constexpr int udp_buffer_size = 2048;
        constexpr int udp_kernel_recv_buffer_size = 1024 * 1024 * 4; // 4MB
        constexpr int udp_kernel_send_buffer_size = 1024 * 1024 * 4; // 4MB

        constexpr int tcp_recv_buffer_size     = 4096;
        constexpr int tcp_send_buffer_size     = 8192;

        constexpr int multicast_data_gc_period_ms = 6 * 1000;

        constexpr std::size_t block_history_max_count = 1024;
    }
}