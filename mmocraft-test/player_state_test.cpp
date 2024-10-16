#include "pch.h"

#include "game/player.h"

TEST(player_state, prepare_transit_properly)
{
    game::PlayerState player_state;

    player_state.prepare_state_transition(game::PlayerState::handshaking, game::PlayerState::handshaked);
    
    EXPECT_EQ(player_state.prev_state(), game::PlayerState::initialized);
    EXPECT_EQ(player_state.state(), game::PlayerState::handshaking);
    EXPECT_EQ(player_state.next_state(), game::PlayerState::handshaked);
}

TEST(player_state, transit_properly)
{
    game::PlayerState player_state;

    player_state.prepare_state_transition(game::PlayerState::handshaking, game::PlayerState::handshaked);
    player_state.transit_state(game::PlayerState::handshaked);

    EXPECT_EQ(player_state.prev_state(), game::PlayerState::handshaking);
    EXPECT_EQ(player_state.state(), game::PlayerState::handshaked);
}