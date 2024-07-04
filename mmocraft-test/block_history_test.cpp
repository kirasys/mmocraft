#include "pch.h"

#include "game/block_history.h"

TEST(BlockHistoryTest, Block_History_Working_Properly_After_Fetch) {
    game::BlockHistory<> block_history;

    block_history.add_record({ 0, 0, 0 }, game::BLOCK_DIRT);
    block_history.add_record({ 0, 0, 1 }, game::BLOCK_DIRT);
    block_history.add_record({ 0, 0, 2 }, game::BLOCK_DIRT);

    std::unique_ptr<std::byte[]> history_data;
    block_history.fetch_serialized_data(history_data);
    block_history.add_record({ 0, 0, 0 }, game::BLOCK_DIRT);

    EXPECT_EQ(block_history.size(), 1);
}