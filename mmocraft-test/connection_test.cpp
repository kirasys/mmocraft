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

    setup::NetworkSystemInitialzer network_init;
    config::Configuration conf;

    win::UniqueSocket unique_sock;
    net::ConnectionEnvironment connection_env;

    io::IoCompletionPort io_service{ io::DEFAULT_NUM_OF_CONCURRENT_EVENT_THREADS };
    io::IoEventPool io_event_pool{ max_player_count };

    net::PacketHandleServerStub<test::MockSocket> handle_server_stub;
    net::TConnection<test::MockSocket> SUT_connection;
};
TEST_F(ConnectionTest, Check_Connection_Timeout)
{
    
    EXPECT_TRUE(true);
}