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
    test::MockDatabase mock;

    net::PacketHandleServerStub handle_server_stub;
    io::IoAcceptEventData io_accept_data;
    io::IoAcceptEvent io_accept_event{ &io_accept_data };

    net::ConnectionEnvironment connection_env{ max_player_count };
    net::ServerCore server_core{ handle_server_stub, connection_env };
};

class MockPacketEvent : public net::PacketEvent
{
public:
    virtual void invoke_handler(ULONG_PTR event_handler_inst) { };

    virtual void invoke_result_handler(void* result_handler_inst) { };

    virtual bool is_exist_pending_packet() const { return false; }

    virtual bool is_exist_pending_result() const { return false; }

    virtual void push_result(net::ConnectionKey, error::ErrorCode error_code) override
    {
        results.push_back(error_code);
    }
    
    std::vector<error::ResultCode> results;
};
TEST_F(MasterServerTest, Handle_Handshake_Correctly)
{
    net::MasterServer SUT_server;

    auto connection_key = server_core.new_connection();
    auto handshake_packet = net::PacketHandshake{ "servername", "motd", net::UserType::NORMAL };

    auto handle_result = SUT_server.handle_handshake_packet(
        *connection_env.try_acquire_descriptor(connection_key),
        handshake_packet);

    EXPECT_TRUE(handle_result.is_packet_handle_success());
}

TEST_F(MasterServerTest, Handle_Deferred_Handshake_With_Duplicate_Login)
{
    net::MasterServer SUT_server;

    auto connection_key = server_core.new_connection();
\
    // links two deferred handshake packets.
    auto handshake_packet = net::PacketHandshake{ "user", "pass", net::UserType::NORMAL };
    auto defer_handshake_packet = net::DeferredPacket<net::PacketHandshake>{
        connection_key,
        handshake_packet};
    auto defer_handshake_packet_next = net::DeferredPacket<net::PacketHandshake>{ 
        connection_key,
        handshake_packet };
    defer_handshake_packet.next = &defer_handshake_packet_next;

    auto packet_event = MockPacketEvent();
    mock.set_outbound_integer(1); // set player id
    SUT_server.handle_deferred_handshake_packet(&packet_event, &defer_handshake_packet);

    EXPECT_EQ(packet_event.results.size(), 2);
    // first login will success then fail to login due to same player ID.
    EXPECT_TRUE(packet_event.results[0].is_login_success() && not packet_event.results[1].is_login_success());
}