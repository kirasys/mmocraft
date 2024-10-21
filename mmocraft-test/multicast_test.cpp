#include "pch.h"

#include "io/multicast_manager.h"

TEST(multicast, expire_entry_properly) {
    io::MulticastManager multicast_manager;
    auto data = std::unique_ptr<std::byte[]>(new std::byte[1024]);
    auto data_size = std::size_t(1024);

    auto& entry = multicast_manager.set_data(io::multicast_tag_id::level_data, std::move(data), data_size);

    EXPECT_FALSE(entry.is_safe_delete());
    entry.update_lifetime(0);
    EXPECT_TRUE(entry.is_safe_delete());
}

TEST(multicast, ref_counting_entry_properly) {
    io::MulticastManager multicast_manager;
    auto data = std::unique_ptr<std::byte[]>(new std::byte[1024]);
    auto data_size = std::size_t(1024);

    auto& entry = multicast_manager.set_data(io::multicast_tag_id::level_data, std::move(data), data_size);
    entry.increase_ref();
    entry.decrease_ref();
    entry.set_reference_count_mode(true);

    EXPECT_TRUE(entry.is_safe_delete());
}

TEST(multicast, clean_entry_only_deactivated) {
    io::MulticastManager multicast_manager;
    auto data = std::unique_ptr<std::byte[]>(new std::byte[1024]);
    auto data_copy = std::unique_ptr<std::byte[]>(new std::byte[1024]);
    auto data_size = std::size_t(1024);

    auto& entry = multicast_manager.set_data(io::multicast_tag_id::level_data, std::move(data), data_size);
    entry.update_lifetime(0);

    EXPECT_EQ(multicast_manager.gc(io::multicast_tag_id::level_data), 0);
    multicast_manager.set_data(io::multicast_tag_id::level_data, std::move(data_copy), data_size); // deactive "data"
    EXPECT_EQ(multicast_manager.gc(io::multicast_tag_id::level_data), 1);
}