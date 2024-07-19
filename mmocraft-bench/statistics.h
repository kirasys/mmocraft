#pragma once

#include <cstdio>

namespace bench
{
    void start_benchmark();

    void on_packet_receive(std::size_t);

    void on_packet_send(std::size_t);

    void print_statistics();
}