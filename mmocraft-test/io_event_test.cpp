#include "pch.h"

#include <cstring>

#include "io/io_event.h"

const std::byte dummy_data[] = { 
    std::byte(1), std::byte(2), std::byte(3), std::byte(4),
    std::byte(5), std::byte(6), std::byte(7), std::byte(8),
};

TEST(io_event, event_data_manipulation_properly) {
    io::IoEventData* io_event_data[] = {
        new io::IoRecvEventData(),
        new io::IoSendEventData(),
        new io::IoSendEventLockFreeData(),
    };

    for (auto event_data : io_event_data) {
        event_data->push(dummy_data, sizeof(dummy_data));
        event_data->pop(1);
        event_data->pop(2);
        event_data->pop(1);

        EXPECT_EQ(event_data->size(), 4);
        EXPECT_EQ(event_data->end() - event_data->begin(), 4);
        EXPECT_EQ(event_data->begin_unused(), event_data->end());
        EXPECT_TRUE(std::memcmp(event_data->begin(), "\x05\x06\x07\x08", 4) == 0);
    }
}

TEST(io_event, trigger_send_data_reset_when_push) {
    io::IoSendEventData io_send_data;

    auto data_begin = io_send_data.begin();
    io_send_data.push(dummy_data, sizeof(dummy_data));
    io_send_data.pop(sizeof(dummy_data));
    auto old_data_begin = io_send_data.begin();
    io_send_data.push(dummy_data, 0);    // trigger to reset head.

    EXPECT_EQ(old_data_begin, data_begin + sizeof(dummy_data));
    EXPECT_EQ(data_begin, io_send_data.begin());
}

TEST(io_event, prevent_send_buffer_overflow) {
    io::IoEventData* io_send_datas[] = {
        new io::IoSendEventData(),
        new io::IoSendEventLockFreeData(),
    };

    for (auto event_data : io_send_datas) {
        std::unique_ptr<std::byte[]> full_data(new std::byte[event_data->capacity()]());;
        bool is_full_data_pushed = event_data->push(full_data.get(), event_data->capacity());
        bool is_overflow_occured = event_data->push(dummy_data, 1);

        EXPECT_TRUE(is_full_data_pushed);
        EXPECT_FALSE(is_overflow_occured);
        EXPECT_EQ(event_data->size(), event_data->capacity());
    }
}

TEST(io_event, non_owning_mode_working_properly) {
    auto io_event_data = new io::IoRecvEventData();
    auto io_event = new io::IoRecvEvent(io_event_data);

    io_event->set_non_owning_mode();
    delete io_event; // should not delete the io_event_data. 
    delete io_event_data;
}

TEST(io_event, multicast_event_ref_counting_working_properly) {

}