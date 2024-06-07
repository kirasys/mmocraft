#include "pch.h"

#include "net/connection_environment.h"
#include "net/server_core.h"

class ServerCoreTest : public testing::Test
{
protected:
    ServerCoreTest()
       : conf{config::get_config().clone()}
    {
       net::Socket::initialize_system();
       conf.server.max_player = 10;
    }

    config::Configuration conf;
    net::PacketHandleServerStub handle_server_stub;
    io::IoAcceptEventData io_accept_data;
    io::IoAcceptEvent io_accept_event{ &io_accept_data };
};
TEST_F(ServerCoreTest, Connection_Creation_Success) { 
    net::ConnectionEnvironment connection_env;
    net::ServerCore server_core{ handle_server_stub, connection_env, conf };

    // server core will create new connection.
    for (unsigned i = 0; i < conf.server.max_player; i++) {
        io_accept_event.accepted_socket = net::create_windows_socket(net::SocketType::TCPv4, WSA_FLAG_OVERLAPPED);
        server_core.handle_io_event(&io_accept_event);
    }
    
    EXPECT_TRUE(server_core.get_last_error().is_strong_success()) 
        << "Unexpected last error: " << server_core.get_last_error().to_string();

    EXPECT_EQ(connection_env.size_of_connections(), 10);
}

TEST_F(ServerCoreTest, Connection_Creation_Exceed) {
    net::ConnectionEnvironment connection_env;
    net::ServerCore server_core{ handle_server_stub, connection_env, conf };

    // server core will create new connection.
    for (unsigned i = 0; i < conf.server.max_player + 1; i++) {
        io_accept_event.accepted_socket = net::create_windows_socket(net::SocketType::TCPv4, WSA_FLAG_OVERLAPPED);
        server_core.handle_io_event(&io_accept_event);
    }

    EXPECT_EQ(server_core.get_last_error().to_error_code(), error::CLIENT_CONNECTION_FULL)
        << "Unexpected last error: " << server_core.get_last_error().to_string();

    // lconnection list never be exceed max player count.
    EXPECT_EQ(connection_env.size_of_connections(), conf.server.max_player);
}

TEST_F(ServerCoreTest, Check_Connection_Timeout) {
    net::ConnectionEnvironment connection_env;
    net::ServerCore server_core{ handle_server_stub, connection_env, conf };

    // server core will create new connection.
    for (unsigned i = 0; i < conf.server.max_player; i++) {
        io_accept_event.accepted_socket = net::create_windows_socket(net::SocketType::TCPv4, WSA_FLAG_OVERLAPPED);
        server_core.handle_io_event(&io_accept_event);
    }

    auto expired_connection = connection_env.get_connection_pointers().front().get();

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
    EXPECT_EQ(connection_env.size_of_connections(), conf.server.max_player - 1);
}