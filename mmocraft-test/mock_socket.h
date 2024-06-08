#pragma once

#include <atomic>

namespace test
{
    class MockSocket
    {
    public:
        MockSocket();

        ~MockSocket();

        void send(char* buf, unsigned n)
        {
            send_buffer = buf;
            send_bytes = n;
            send_call_counter++;
        }

        void recv(char* buf, unsigned n)
        {
            recv_buffer = buf;
            recv_bytes = n;
            recv_call_counter++;
        }

        auto get_send_times() const
        {
            return send_call_counter.load();
        }

        auto get_recv_times() const
        {
            return recv_call_counter.load();
        }

        auto get_send_bytes() const
        {
            return send_bytes;
        }

        auto get_recv_bytes() const
        {
            return recv_bytes;
        }

        auto get_send_buffer() const
        {
            return send_buffer;
        }

        auto get_recv_buffer() const
        {
            return recv_buffer;
        }

    private:
        std::atomic<unsigned> send_call_counter{ 0 };
        char* send_buffer;
        unsigned send_bytes = 0;

        std::atomic<unsigned> recv_call_counter{ 0 };
        char* recv_buffer;
        unsigned recv_bytes = 0;
    };

    
}