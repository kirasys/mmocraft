#include "pch.h"
#include "server_core.h"

#include <cassert>

#include "config/config.h"
#include "proto/config.pb.h"
#include "logging/error.h"
#include "net/connection_environment.h"

namespace net
{
    ServerCore::ServerCore
        (net::PacketHandleServer& a_packet_handle_server, net::ConnectionEnvironment& a_connection_env, io::IoService& a_io_service)
        : packet_handle_server{ a_packet_handle_server }
        , connection_env{ a_connection_env }
        , connection_env_task{ &connection_env }
        , _listen_sock{ net::SocketProtocol::TCPv4Overlapped }
        , io_service{ a_io_service }
    {	
        io_service.register_event_source(_listen_sock.get_handle(), /*.event_handler = */ this);

        /// Schedule interval tasks.

        connection_env_task.schedule(
            util::TaskTag::CLEAN_CONNECTION,
            &ConnectionEnvironment::cleanup_expired_connection,
            util::MilliSecond(2000));

        _state = ServerCore::State::Initialized;
    }

    net::ConnectionKey ServerCore::new_connection(win::UniqueSocket &&client_sock)
    {
        auto connection_key = ConnectionKey(connection_env.get_unused_connection_id(), util::current_monotonic_tick32());

        std::unique_ptr<net::Connection> connection_ptr(new net::Connection(
            packet_handle_server,
            connection_key,
            connection_env,
            std::move(client_sock),
            io_service
        ));

        connection_env.on_connection_create(connection_key, std::move(connection_ptr));
        return connection_key;
    }

    void ServerCore::start_network_io_service()
    {
        auto& server_conf = config::get_server_config();
        auto& system_conf = config::get_system_config();

        _listen_sock.bind(server_conf.ip(), server_conf.port());
        _listen_sock.listen();
        start_accept();

        CONSOLE_LOG(info) << "Listening to " << server_conf.ip() << ':' << server_conf.port() << "...\n";

        for (unsigned i = 0; i < system_conf.num_of_processors() * 2; i++)
            io_service.spawn_event_loop_thread().detach();
    }

    void ServerCore::start_accept()
    {
        _state = ServerCore::State::Running;

        if (auto error_code = _listen_sock.accept(io_accept_event)) {
            LOG(error) << "Fail to request accept: " << error_code;
        }
    }

    /** 
     *  Event handler interface
     */

    void ServerCore::on_error()
    {
        _state = ServerCore::State::Stopped;
    }

    void ServerCore::on_complete(io::IoAcceptEvent* event)
    {
        LOG_IF(error, not last_error_code.is_success()) 
            << "Fail to accpet new connection: " << last_error_code;

        start_accept();
    }

    std::size_t ServerCore::handle_io_event(io::IoAcceptEvent* event)
    {
        connection_env_task.process_task(util::TaskTag::CLEAN_CONNECTION);

        // creates unique accept socket first to avoid resource leak.
        auto client_socket = win::UniqueSocket(event->accepted_socket);

        if (connection_env.size_of_connections() >= connection_env.size_of_max_connections()) {
            last_error_code = error::CLIENT_CONNECTION_FULL;
            return 0;
        }

        // inherit the properties of the listen socket.
        win::Socket listen_sock = _listen_sock.get_handle();
        if (SOCKET_ERROR == ::setsockopt(client_socket,
            SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
            reinterpret_cast<char*>(&listen_sock), sizeof(listen_sock))) {
            LOG(error) << "setsockopt(SO_UPDATE_ACCEPT_CONTEXT) failed but suppressed.";
        }

        // add a client to the server.
        new_connection(std::move(client_socket));
        last_error_code = error::SUCCESS;
        return 0;
    }
}