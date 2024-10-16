#include "pch.h"

#include "game/history_buffer.h"

TEST(history_buffer, block_history_working_properly)
{
    game::BlockHistory block_history;

    block_history.add_record({ 1, 2, 3 }, game::block_id::dirt);
    block_history.add_record({ 4, 5, 6 }, game::block_id::air);
    block_history.add_record({ 7, 8, 9 }, game::block_id::bedrock);

    block_history.snapshot();

    EXPECT_EQ(block_history.size(), 3);
    EXPECT_EQ(block_history.get_record(0).block_id, std::byte(game::block_id::dirt));
    EXPECT_EQ(block_history.get_record(1).block_id, std::byte(game::block_id::air));
    EXPECT_EQ(block_history.get_record(2).block_id, std::byte(game::block_id::bedrock));
}

TEST(history_buffer, record_available_only_after_snapshot)
{
    game::BlockHistory block_history;

    block_history.add_record({ 1, 2, 3 }, game::block_id::dirt);
    block_history.add_record({ 4, 5, 6 }, game::block_id::air);
    block_history.add_record({ 7, 8, 9 }, game::block_id::bedrock);

    EXPECT_EQ(block_history.size(), 0);
}

TEST(history_buffer, clear_snapshot_properly)
{
    game::BlockHistory block_history;

    block_history.add_record({ 1, 2, 3 }, game::block_id::dirt);
    block_history.add_record({ 4, 5, 6 }, game::block_id::air);
    block_history.add_record({ 7, 8, 9 }, game::block_id::bedrock);

    block_history.snapshot();
    block_history.clear_snapshot();

    EXPECT_EQ(block_history.size(), 0);
}