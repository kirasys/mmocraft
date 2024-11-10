#include "statistics.h"

#include <atomic>
#include <iostream>
#include <mutex>

#include "net/packet_extension.h"
#include "util/time_util.h"

namespace
{
    std::size_t bench_start_tick = 0;

    std::atomic<std::size_t> total_packet_send_count{ 0 };
    std::atomic<std::size_t> total_packet_receive_count{ 0 };

    std::atomic<std::size_t> total_packet_data_sended{ 0 };
    std::atomic<std::size_t> total_packet_data_received{ 0 };

    std::atomic<std::size_t> total_ping_received{ 0 };   
    std::atomic<std::size_t> total_ping_rtt{ 0 };

    std::mutex max_ping_update_lock;
    std::atomic<std::size_t> max_ping_rtt{ 0 };
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

    void on_ping_packet_receive(const std::byte* packet_data)
    {
        // update ping rtt statistics
        net::PacketExtPing packet(packet_data);
        auto ping_rtt = packet.get_rtt_ns();
        
        total_ping_rtt.fetch_add(ping_rtt, std::memory_order_relaxed);
        if (max_ping_rtt.load(std::memory_order_relaxed) < ping_rtt) {
            std::lock_guard guard(max_ping_update_lock);
            // check if max_ping_rtt is still less than ping_rtt.
            if (max_ping_rtt.load(std::memory_order_relaxed) < ping_rtt)
                max_ping_rtt.store(ping_rtt, std::memory_order_relaxed);
        }

        // update ping count statistics.
        total_ping_received.fetch_add(1, std::memory_order_relaxed);
    }

    void print_statistics()
    {
        if (double elapesd_seconds = (util::current_monotonic_tick() - bench_start_tick) / 1000.0) {
            std::cout << "Total sended packet count   : " << total_packet_send_count.load(std::memory_order_relaxed) << '\n';
            std::cout << " - Throughput per second    : " << total_packet_send_count.load(std::memory_order_relaxed) / elapesd_seconds << '\n';

            std::cout << "Total sended packet bytes   : " << total_packet_data_sended.load(std::memory_order_relaxed) << '\n';
            std::cout << " - Throughput per second (MB)  : " << total_packet_data_sended.load(std::memory_order_relaxed) / elapesd_seconds / 1024 / 1024 << '\n';

            std::cout << "Total received packet count : " << total_packet_receive_count.load(std::memory_order_relaxed) << '\n';
            std::cout << " - Throughput per second    : " << total_packet_receive_count.load(std::memory_order_relaxed) / elapesd_seconds << '\n';

            std::cout << "Total received packet bytes : " << total_packet_data_received.load(std::memory_order_relaxed) << '\n';
            std::cout << " - Throughput per second (MB) : " << total_packet_data_received.load(std::memory_order_relaxed) / elapesd_seconds / 1024 / 1024 << '\n';
        }

        if (auto ping_count = total_ping_received.load(std::memory_order_relaxed)) {
            std::cout << "Average ping RTT (ns): "
                << total_ping_rtt.load(std::memory_order_relaxed) / ping_count << '\n';
            std::cout << "Maximum ping RTT (ns): "
                << max_ping_rtt.load(std::memory_order_relaxed);
        }
    }
}