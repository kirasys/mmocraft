#include "pch.h"

#include <cstring>

#include "io/io_event.h"

const std::byte dummy_data[] = { 
    std::byte(1), std::byte(2), std::byte(3), std::byte(4),
    std::byte(5), std::byte(6), std::byte(7), std::byte(8),
};

TEST(IoEventTest, Recv_Data_Working_Properly) {
    io::IoRecvEventData io_recv_data;

    io_recv_data.push(dummy_data, sizeof(dummy_data));
    io_recv_data.pop(1);
    io_recv_data.pop(2);
    io_recv_data.pop(1);

    EXPECT_EQ(io_recv_data.size(), 4);
    EXPECT_EQ(io_recv_data.end() - io_recv_data.begin(), 4);
    EXPECT_EQ(io_recv_data.begin_unused(), io_recv_data.end());
    EXPECT_TRUE(std::memcmp(io_recv_data.begin(), "\x05\x06\x07\x08", 4) == 0);
}

TEST(IoEventTest, Event_Data_Working_Properly) {
    io::IoEventData* io_send_datas[] = {
        new io::IoRecvEventData(),
        new io::IoSendEventData(),
        new io::IoSendEventLockFreeData(),
    };

    for (auto send_data : io_send_datas) {
        send_data->push(dummy_data, sizeof(dummy_data));
        send_data->pop(1);
        send_data->pop(2);
        send_data->pop(1);

        EXPECT_EQ(send_data->size(), 4);
        EXPECT_EQ(send_data->end() - send_data->begin(), 4);
        EXPECT_EQ(send_data->begin_unused(), send_data->end());
        EXPECT_TRUE(std::memcmp(send_data->begin(), "\x05\x06\x07\x08", 4) == 0);
    }
}

TEST(IoEventTest, Send_Data_Head_Reset_When_Push) {
    io::IoSendEventData io_send_data;

    auto data_begin = io_send_data.begin();
    io_send_data.push(dummy_data, sizeof(dummy_data));
    io_send_data.pop(sizeof(dummy_data));
    auto data_end = io_send_data.begin();
    io_send_data.push(dummy_data, 0);    // trigger to reset head.

    EXPECT_EQ(data_end, data_begin + sizeof(dummy_data));
    EXPECT_EQ(data_begin, io_send_data.begin());
}

TEST(IoEventTest, Send_Data_Cant_Overflow) {
    io::IoSendEventData io_send_data;

    std::byte full_data[io::SEND_BUFFER_SIZE] = {};
    bool full_data_pushed = io_send_data.push(full_data, sizeof(full_data));
    bool overflowed = io_send_data.push(dummy_data, 1);

    EXPECT_TRUE(full_data_pushed);
    EXPECT_FALSE(overflowed);
    EXPECT_EQ(io_send_data.size(), sizeof(full_data));
}

TEST(IoEventTest, Lockfree_Send_Data_Cant_Overflow) {
    io::IoSendEventLockFreeData io_send_data;

    std::byte full_data[io::CONCURRENT_SEND_BUFFER_SIZE] = {};
    bool full_data_pushed = io_send_data.push(full_data, sizeof(full_data));
    bool overflowed = io_send_data.push(dummy_data, 1);

    EXPECT_TRUE(full_data_pushed);
    EXPECT_FALSE(overflowed);
    EXPECT_EQ(io_send_data.size(), sizeof(full_data));
}