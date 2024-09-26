#pragma once

#include <cstdio>
#include <cstddef>

namespace bench
{
    void start_benchmark();

    void on_packet_receive(std::size_t);

    void on_packet_send(std::size_t);

    void on_ping_packet_receive(const std::byte* packet_data);

    void print_statistics();
}