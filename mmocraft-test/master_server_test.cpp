#include "pch.h"
#include <vector>

#include "mock_database.h"
#include "net/master_server.h"

class MasterServerTest : public testing::Test
{
protected:
    MasterServerTest()
        : conf{ config::get_config().clone() }
        , connection_descriptor{nullptr, win::UniqueSocket(), nullptr, &io_send_event, &io_deferred_send_event }
    {
        conf.server.max_player = 5;
    }

    test::MockDatabase mock;
    config::Configuration conf;

    io::IoSendEventData io_send_data;
    io::IoSendEvent io_send_event{ &io_send_data };

    io::IoSendEventData io_deferred_send_data;
    io::IoSendEvent io_deferred_send_event{ &io_deferred_send_data };

    net::Connection::Descriptor connection_descriptor;
};

class MockPacketEvent : public net::PacketEvent
{
public:
    virtual void invoke_handler(ULONG_PTR event_handler_inst) { };

    virtual void invoke_result_handler(void* result_handler_inst) { };

    virtual bool is_exist_pending_packet() const { return false; }

    virtual bool is_exist_pending_result() const { return false; }

    virtual void push_result(net::Connection::Descriptor*, error::ErrorCode error_code) override
    {
        results.push_back(error_code);
    }
    
    std::vector<error::ResultCode> results;
};
TEST_F(MasterServerTest, Handle_Handshake_Correctly)
{
    net::MasterServer SUT_server{ conf };
    auto handshake_packet = net::PacketHandshake{ "servername", "motd", net::UserType::NORMAL };

    auto handle_result = SUT_server.handle_handshake_packet(connection_descriptor, handshake_packet);

    EXPECT_TRUE(handle_result.is_packet_handle_success());
}

TEST_F(MasterServerTest, Handle_Deferred_Handshake_With_Duplicate_Login)
{
    net::MasterServer SUT_server{ conf };

    auto packet_event = MockPacketEvent();
    // links two deferred handshake packets.
    auto handshake_packet = net::PacketHandshake{ "user", "pass", net::UserType::NORMAL };
    auto defer_handshake_packet = net::DeferredPacket<net::PacketHandshake>{ &connection_descriptor, handshake_packet };
    auto defer_handshake_packet_next = net::DeferredPacket<net::PacketHandshake>{ &connection_descriptor, handshake_packet };
    defer_handshake_packet.next = &defer_handshake_packet_next;

    mock.set_outbound_integer(1); // set player id
    SUT_server.handle_deferred_handshake_packet(&packet_event, &defer_handshake_packet);

    EXPECT_EQ(packet_event.results.size(), 2);
    // first login will success then fail to login due to same player ID.
    EXPECT_TRUE(packet_event.results[0].is_login_success() && not packet_event.results[1].is_login_success());
}

TEST_F(MasterServerTest, Handle_Deferred_Handshake_Result_Correctly)
{
    net::MasterServer SUT_server{ conf };
    auto defer_handshake_result = net::DeferredPacketResult{ &connection_descriptor, error::PACKET_RESULT_SUCCESS_LOGIN };
    
    connection_descriptor.associate_game_player(1, game::PlayerType::GUEST, "user", "pass");
    SUT_server.handle_deferred_handshake_packet_result(&defer_handshake_result);

    auto sended_packet_id = net::PacketID(io_deferred_send_data.data()[0]);
    EXPECT_EQ(sended_packet_id, net::PacketID::Handshake);
    EXPECT_TRUE(io_deferred_send_data.size() > 0);
}

TEST_F(MasterServerTest, Handle_Deferred_Handshake_Fail_Result)
{
    net::MasterServer SUT_server{ conf };
    auto defer_handshake_result = net::DeferredPacketResult{ &connection_descriptor, error::PACKET_RESULT_FAIL_LOGIN };

    connection_descriptor.associate_game_player(1, game::PlayerType::GUEST, "user", "pass");
    SUT_server.handle_deferred_handshake_packet_result(&defer_handshake_result);

    auto sended_packet_id = net::PacketID(io_deferred_send_data.data()[0]);
    EXPECT_EQ(sended_packet_id, net::PacketID::DisconnectPlayer);
    EXPECT_TRUE(io_deferred_send_data.size() > 0);
    EXPECT_FALSE(connection_descriptor.is_online());
}