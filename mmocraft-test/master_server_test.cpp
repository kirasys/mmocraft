#include "pch.h"
#include <vector>

#include "config/config.h"
#include "proto/config.pb.h"
#include "mock_database.h"
#include "net/master_server.h"
#include "system_initializer.h"

constexpr unsigned max_player_count = 5;

class MasterServerTest : public testing::Test
{
protected:
    MasterServerTest()
        : system_init(
            []() {
                net::Socket::initialize_system();
                config::set_default_configuration();
                config::get_server_config().set_max_player(max_player_count);
            }
        )
    {

    }

    setup::SystemInitialzer system_init;
    test::MockDatabase mock_db;

    io::IoCompletionPort io_service;

    net::PacketHandleServerStub handle_server_stub;

    net::ConnectionEnvironment connection_env{ max_player_count };

    net::ServerCore server_core{ handle_server_stub, connection_env, io_service };

    net::MasterServer SUT_server{ connection_env, io_service };
};

TEST_F(MasterServerTest, Handle_Handshake_Correctly)
{
    auto connection_key = server_core.new_connection();
    auto handshake_packet = net::PacketHandshake{ "servername", "motd", net::UserType::NORMAL };

    auto handle_result = SUT_server.handle_handshake_packet(
        *connection_env.try_acquire_connection(connection_key),
        handshake_packet);

    EXPECT_TRUE(handle_result.is_packet_handle_success());
}

TEST_F(MasterServerTest, Handle_Deferred_Handshake_With_Duplicate_Login)
{
    // Assignment
    auto connection_key = server_core.new_connection();
    auto connection_key_second = server_core.new_connection();

    // links two deferred handshake packets.
    auto handshake_packet = net::PacketHandshake{ "user", "pass", net::UserType::NORMAL };
    auto defer_handshake_packet = net::DeferredPacket<net::PacketHandshake>{
        connection_key,
        handshake_packet};
    auto defer_handshake_packet_next = net::DeferredPacket<net::PacketHandshake>{ 
        connection_key_second,
        handshake_packet };
    defer_handshake_packet.next = &defer_handshake_packet_next;

    io::SimpleTask<net::MasterServer> dummy_task{ nullptr, nullptr };

    /// Action
    mock_db.set_outbound_integer(1); // make both player identity to 1.
    SUT_server.handle_deferred_handshake_packet(&dummy_task, &defer_handshake_packet);

    // Assert
    ASSERT_TRUE(connection_env.try_acquire_connection(connection_key)->get_connected_player() != nullptr);
    auto player = connection_env.try_acquire_connection(connection_key)->get_connected_player();
    EXPECT_EQ(player->state(), game::PlayerState::Handshake_Completed);
    EXPECT_TRUE(connection_env.try_acquire_connection(connection_key_second) == nullptr); // offlined
}