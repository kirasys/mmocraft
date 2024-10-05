#pragma once

#include <atomic>
#include <cstddef>
#include <cstdlib>

#include "util/string_util.h"

namespace util
{
    class DoubleBuffering
    {
    public:
        class Buffer
        {
        public:
            bool push(const std::byte* data, std::size_t data_size)
            {
                auto before_size = buf_size.fetch_add(data_size, std::memory_order_relaxed);
                if (before_size + data_size > sizeof(buf)) {
                    buf_size.fetch_sub(data_size, std::memory_order_relaxed);
                    return false;
                }

                std::memcpy(buf + before_size, data, data_size);
                return true;
            }

            std::byte* begin()
            {
                return buf;
            }

            const std::byte* begin() const
            {
                return buf;
            }

            std::size_t size() const
            {
                return buf_size.load(std::memory_order_relaxed);
            }

            void clear()
            {
                buf_size.store(0, std::memory_order_relaxed);
            }

        private:
            std::atomic<std::size_t> buf_size;
            std::byte buf[0x2000];
        };

        void snapshot()
        {
            switch_buffer();
        }

        void clear_snapshot()
        {
            output_buffer().clear();
        }

        util::byte_view get_snapshot_data() const
        {
            auto& buffer = output_buffer();
            return { buffer.begin(), buffer.size() };
        }

        bool has_live_data() const
        {
            return input_buffer().size() > 0;
        }

    protected:

        void switch_buffer()
        {
            input_buf_index ^= 1;
        }

        Buffer& input_buffer()
        {
            return bufs[input_buffer_index()];
        }

        const Buffer& input_buffer() const
        {
            return bufs[input_buffer_index()];
        }

        Buffer& output_buffer()
        {
            return bufs[output_buffer_index()];
        }

        const Buffer& output_buffer() const
        {
            return bufs[output_buffer_index()];
        }

    private:
        inline int input_buffer_index() const
        {
            return input_buf_index.load(std::memory_order_relaxed);
        }

        inline int output_buffer_index() const
        {
            return input_buf_index.load(std::memory_order_relaxed) ^ 1;
        }

        std::atomic<int> input_buf_index{ 0 };

        Buffer bufs[2];
    };
}