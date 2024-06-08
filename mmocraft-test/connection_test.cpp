#include "pch.h"

#include "system_initializer.h"
#include "config/config.h"
#include "net/connection.h"
#include "net/connection_environment.h"

#include "mock_socket.h"

constexpr unsigned max_player_count = 10;

class ConnectionTest : public testing::Test
{
protected:
    ConnectionTest()
        : network_init{}
        , conf{ config::get_config().clone() }
        , unique_sock{net::create_windows_socket(net::SocketProtocol::TCPv4, WSA_FLAG_OVERLAPPED) }
        , SUT_connection{ handle_server_stub, connection_env, std::move(unique_sock), io_service, io_event_pool }
    {
        conf.server.max_player = max_player_count;
    }

    test::MockSocket mock;

    setup::NetworkSystemInitialzer network_init;
    config::Configuration conf;

    win::UniqueSocket unique_sock;
    net::ConnectionEnvironment connection_env;

    io::IoCompletionPort io_service{ io::DEFAULT_NUM_OF_CONCURRENT_EVENT_THREADS };
    io::IoEventPool io_event_pool{ max_player_count };

    net::PacketHandleServerStub handle_server_stub;
    net::Connection SUT_connection;
};
TEST_F(ConnectionTest, Request_Recv_Event_Correctly)
{
    io::IoRecvEventData io_recv_data;
    io::IoRecvEvent io_recv_event(&io_recv_data);

    auto recv_buffer_start = reinterpret_cast<char*>(io_recv_data.data());
    auto recv_buffer_space = io::RECV_BUFFER_SIZE;

    SUT_connection.descriptor.activate_receive_cycle(&io_recv_event);

    EXPECT_TRUE(io_recv_event.is_processing);
    EXPECT_EQ(mock.get_recv_bytes(), recv_buffer_space);
    EXPECT_EQ(mock.get_recv_buffer(), recv_buffer_start);

    // if receives 100 bytes
    io_recv_data.push(nullptr, 100);
    SUT_connection.descriptor.activate_receive_cycle(&io_recv_event);

    EXPECT_TRUE(io_recv_event.is_processing);
    EXPECT_EQ(mock.get_recv_bytes(), recv_buffer_space - 100);
    EXPECT_EQ(mock.get_recv_buffer(), recv_buffer_start + 100);
}

TEST_F(ConnectionTest, Request_Send_Event_Correctly)
{
    io::IoSendEventData io_send_data;
    io::IoSendEvent io_send_event(&io_send_data);

    auto send_buffer_start = reinterpret_cast<char*>(io_send_data.data());
    auto send_buffer_space = io::SEND_BUFFER_SIZE;

    // send 100 bytes.
    io_send_data.commit(100);
    SUT_connection.descriptor.activate_send_cycle(&io_send_event);

    EXPECT_TRUE(io_send_event.is_processing);
    EXPECT_EQ(mock.get_send_bytes(), 100);
    EXPECT_EQ(mock.get_send_buffer(), send_buffer_start);

    // complete sending 50 bytes.
    io_send_data.pop(50);
    SUT_connection.descriptor.activate_send_cycle(&io_send_event);

    EXPECT_TRUE(io_send_event.is_processing);
    EXPECT_EQ(mock.get_send_bytes(), 50);
    EXPECT_EQ(mock.get_send_buffer(), send_buffer_start + 50);
}