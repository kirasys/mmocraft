#include "statistics.h"

#include <atomic>
#include <iostream>

#include "util/time_util.h"

namespace
{
    std::size_t bench_start_tick = 0;

    std::atomic<std::size_t> total_packet_send_count{ 0 };
    std::atomic<std::size_t> total_packet_receive_count{ 0 };

    std::atomic<std::size_t> total_packet_data_sended{ 0 };
    std::atomic<std::size_t> total_packet_data_received{ 0 };
}

namespace bench
{
    void start_benchmark()
    {
        bench_start_tick = util::current_monotonic_tick();
    }

    void on_packet_send(std::size_t data_size)
    {
        total_packet_send_count.fetch_add(1, std::memory_order_relaxed);
        total_packet_data_sended.fetch_add(data_size, std::memory_order_relaxed);
    }

    void on_packet_receive(std::size_t data_size)
    {
        total_packet_receive_count.fetch_add(1, std::memory_order_relaxed);
        total_packet_data_received.fetch_add(data_size, std::memory_order_relaxed);
    }

    void print_statistics()
    {
        double elapesd_seconds = (util::current_monotonic_tick() - bench_start_tick) / 1000.0;
        std::cout << "Total sended packet count   : " << total_packet_send_count.load(std::memory_order_relaxed) << '\n';
        std::cout << " - Throughput per second    : " << total_packet_send_count.load(std::memory_order_relaxed) / elapesd_seconds << '\n';
        
        std::cout << "Total sended packet bytes   : " << total_packet_data_sended.load(std::memory_order_relaxed) << '\n';
        std::cout << " - Throughput per second    : " << total_packet_data_sended.load(std::memory_order_relaxed) / elapesd_seconds << '\n';

        std::cout << "Total received packet count : " << total_packet_receive_count.load(std::memory_order_relaxed) << '\n';
        std::cout << " - Throughput per second    : " << total_packet_receive_count.load(std::memory_order_relaxed) / elapesd_seconds << '\n';

        std::cout << "Total received packet bytes : " << total_packet_data_received.load(std::memory_order_relaxed) << '\n';
        std::cout << " - Throughput per second    : " << total_packet_data_received.load(std::memory_order_relaxed) / elapesd_seconds << '\n';
    }
}