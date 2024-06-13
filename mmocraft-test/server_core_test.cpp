#include "pch.h"

#include "config/config.h"
#include "proto/config.pb.h"
#include "net/connection_environment.h"
#include "net/server_core.h"

constexpr unsigned max_player_count = 5;

class ServerCoreTest : public testing::Test
{
protected:
    ServerCoreTest()
    {
       net::Socket::initialize_system();
       config::set_default_configuration();
       config::get_server_config().set_max_player(max_player_count);
    }

    net::PacketHandleServerStub handle_server_stub;
    io::IoAcceptEventData io_accept_data;
    io::IoAcceptEvent io_accept_event{ &io_accept_data };

    net::ConnectionEnvironment connection_env{ max_player_count };
};

TEST_F(ServerCoreTest, Connection_Creation_Success) { 
    net::ServerCore SUT_server{ handle_server_stub, connection_env };
    bool is_success_create_max_player = true;

    // server core will create new connection.
    for (unsigned i = 0; i < max_player_count; i++) {
        io_accept_event.accepted_socket = net::create_windows_socket(net::SocketProtocol::TCPv4, WSA_FLAG_OVERLAPPED);
        SUT_server.handle_io_event(&io_accept_event);
        is_success_create_max_player &= SUT_server.get_last_error().is_success();
    }

    EXPECT_TRUE(is_success_create_max_player);
    EXPECT_EQ(connection_env.size_of_connections(), max_player_count);
}

TEST_F(ServerCoreTest, Connection_Creation_Exceed) {
    net::ServerCore SUT_server{ handle_server_stub, connection_env };

    // try to create more than maximum.
    for (unsigned i = 0; i < max_player_count + 1; i++) {
        io_accept_event.accepted_socket = net::create_windows_socket(net::SocketProtocol::TCPv4, WSA_FLAG_OVERLAPPED);
        SUT_server.handle_io_event(&io_accept_event);
    }

    EXPECT_EQ(SUT_server.get_last_error().to_error_code(), error::CLIENT_CONNECTION_FULL)
        << "Unexpected last error: " << SUT_server.get_last_error().to_string();
    // lconnection list never be exceed max player count.
    EXPECT_EQ(connection_env.size_of_connections(), max_player_count);
}

TEST_F(ServerCoreTest, Check_Connection_Timeout) {;
    net::ServerCore SUT_server{ handle_server_stub, connection_env };

    // server core will create new connection.
    for (unsigned i = 0; i < max_player_count; i++) {
        io_accept_event.accepted_socket = net::create_windows_socket(net::SocketProtocol::TCPv4, WSA_FLAG_OVERLAPPED);
        SUT_server.handle_io_event(&io_accept_event);
    }

    auto expired_connection = connection_env.get_connection(0);

    // trigger connection timeout.
    expired_connection->descriptor.update_last_interaction_time(1);
    auto before_cleanup_online_status = expired_connection->descriptor.is_online();
    connection_env.cleanup_expired_connection();
    auto after_cleanup_online_status = expired_connection->descriptor.is_online();

    // trigger connection deletion.
    expired_connection->descriptor.set_offline(1);
    connection_env.cleanup_expired_connection();       // coonection will be deleted
    
    EXPECT_TRUE(before_cleanup_online_status);
    EXPECT_FALSE(after_cleanup_online_status);
    EXPECT_EQ(connection_env.size_of_connections(), max_player_count - 1);
}