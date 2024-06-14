#include "pch.h"

#include "system_initializer.h"
#include "config/config.h"
#include "proto/config.pb.h"
#include "net/connection.h"
#include "net/connection_environment.h"
#include "net/server_core.h"

#include "mock_socket.h"

constexpr unsigned max_player_count = 5;

class ConnectionTest : public testing::Test
{
protected:
    ConnectionTest()
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

    test::MockSocket mock;

    net::PacketHandleServerStub handle_server_stub;
    io::IoAcceptEventData io_accept_data;
    io::IoAcceptEvent io_accept_event{ &io_accept_data };

    net::ConnectionEnvironment connection_env{ max_player_count };
    net::ServerCore server_core{ handle_server_stub, connection_env };
};

TEST_F(ConnectionTest, Emit_Recv_Event_Correctly)
{
    auto connection_key = server_core.new_connection();
    auto SUT_connection_descriptor = connection_env.try_acquire_descriptor(connection_key);

    io::IoRecvEventData io_recv_data;
    io::IoRecvEvent io_recv_event(&io_recv_data);

    auto recv_buffer_start = reinterpret_cast<char*>(io_recv_data.data());
    auto recv_buffer_space = io::RECV_BUFFER_SIZE;

    SUT_connection_descriptor->emit_receive_event(&io_recv_event);

    EXPECT_TRUE(io_recv_event.is_processing);
    EXPECT_EQ(mock.get_recv_bytes(), recv_buffer_space);
    EXPECT_EQ(mock.get_recv_buffer(), recv_buffer_start);

    // if receives only 100 bytes
    io_recv_data.push(nullptr, 100);
    SUT_connection_descriptor->emit_receive_event(&io_recv_event);

    EXPECT_TRUE(io_recv_event.is_processing);
    EXPECT_EQ(mock.get_recv_bytes(), recv_buffer_space - 100);
    EXPECT_EQ(mock.get_recv_buffer(), recv_buffer_start + 100);
}

TEST_F(ConnectionTest, Emit_Recv_Event_Fail_Insuffient_Buffer)
{
    auto connection_key = server_core.new_connection();
    auto SUT_connection_descriptor = connection_env.try_acquire_descriptor(connection_key);

    io::IoRecvEventData io_recv_data;
    io::IoRecvEvent io_recv_event(&io_recv_data);

    io_recv_data.push(nullptr, io::RECV_BUFFER_SIZE);
    SUT_connection_descriptor->emit_receive_event(&io_recv_event);

    EXPECT_TRUE(not io_recv_event.is_processing);
    EXPECT_EQ(mock.get_recv_times(), 0 + 1);
}

TEST_F(ConnectionTest, Send_Event_Correctly)
{
    auto connection_key = server_core.new_connection();
    auto SUT_connection_descriptor = connection_env.try_acquire_descriptor(connection_key);

    io::IoSendEventData io_send_data;
    io::IoSendEvent io_send_event(&io_send_data);

    auto send_buffer_start = reinterpret_cast<char*>(io_send_data.data());
    auto send_buffer_space = io::SEND_BUFFER_SIZE;

    // send 100 bytes.
    io_send_data.commit(100);
    SUT_connection_descriptor->emit_send_event(&io_send_event);

    EXPECT_TRUE(io_send_event.is_processing);
    EXPECT_EQ(mock.get_send_bytes(), 100);
    EXPECT_EQ(mock.get_send_buffer(), send_buffer_start);

    // complete sending 50 bytes.
    io_send_data.pop(50);
    SUT_connection_descriptor->emit_send_event(&io_send_event);

    EXPECT_TRUE(io_send_event.is_processing);
    EXPECT_EQ(mock.get_send_bytes(), 50);
    EXPECT_EQ(mock.get_send_buffer(), send_buffer_start + 50);
}

TEST_F(ConnectionTest, Handle_Recv_Event_Correctly)
{
    auto connection_key = server_core.new_connection();
    auto SUT_connection = connection_env.try_acquire_connection(connection_key);

    io::IoRecvEventData io_recv_data;
    io::IoRecvEvent io_recv_event(&io_recv_data);

    std::byte set_block_packet[]{ std::byte(0x05),                    // packet id
                                  std::byte(0x01), std::byte(0x00),   // x
                                  std::byte(0x02), std::byte(0x00),   // y
                                  std::byte(0x03), std::byte(0x00),   // z
                                  std::byte(1),                       // mode
                                  std::byte(0)};                      // block_type

    // write three set block packets to the buffer.
    auto recv_buffer_start = io_recv_data.data();
    for (int i = 0; i < 3; i++)
        std::memcpy(recv_buffer_start + i * sizeof(set_block_packet), set_block_packet, sizeof(set_block_packet));

    io_recv_event.invoke_handler(*SUT_connection, sizeof(set_block_packet) * 3);

    EXPECT_TRUE(SUT_connection->get_last_error().is_success())
        << "Unexpected error:" << SUT_connection->get_last_error().to_string();
    EXPECT_EQ(mock.get_recv_times(), 1 + 1);
    // expect all data have been consumed.
    EXPECT_EQ(io_recv_data.size(), 0);
}

TEST_F(ConnectionTest, Handle_Recv_Event_Partial_Packet_Correctly)
{
    auto connection_key = server_core.new_connection();
    auto SUT_connection = connection_env.try_acquire_connection(connection_key);

    io::IoRecvEventData io_recv_data;
    io::IoRecvEvent io_recv_event(&io_recv_data);

    std::byte set_block_partial_packet[]{ std::byte(0x05),            // packet id
                                  std::byte(0x01), std::byte(0x00),   // x
                                  std::byte(0x02), std::byte(0x00),   // y
                                  std::byte(0x03), std::byte(0x00),   // z
                                  std::byte(1),                       // mode
                               /* std::byte(0) */};                   // block_type

    // write three set block packets to the buffer.
    auto recv_buffer_start = io_recv_data.data();
    std::memcpy(recv_buffer_start, set_block_partial_packet, sizeof(set_block_partial_packet));

    io_recv_event.invoke_handler(*SUT_connection, sizeof(set_block_partial_packet));

    // expect going to receive nonetheless insuffient packet data error.
    EXPECT_EQ(mock.get_recv_times(), 1 + 1);
    // expect packet data is unchanged.
    EXPECT_EQ(io_recv_data.size(), sizeof(set_block_partial_packet)); 
}