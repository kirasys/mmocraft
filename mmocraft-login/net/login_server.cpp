#include "login_server.h"

#include <config/config.h>
#include <database/query.h>
#include <net/packet.h>
#include <proto/generated/protocol.pb.h>
#include <util/common_util.h>
#include <logging/logger.h>

#include "../config/config.h"

namespace
{

    std::array<login::net::LoginServer::handler_type, 0x100> message_handler_table = [] {
        std::array<login::net::LoginServer::handler_type, 0x100> arr{};
        arr[::net::MessageID::Login_PacketHandshake] = &login::net::LoginServer::handle_handshake_packet;
        arr[::net::MessageID::Login_PlayerLogout] = &login::net::LoginServer::handle_player_logout_message;
        return arr;
    }();
}

namespace login
{
namespace net
{
    LoginServer::LoginServer()
        : server_core{ this, &message_handler_table }
        , interval_tasks{ this }
    {
        interval_tasks.schedule(
            ::util::TaskTag::ANNOUNCE_SERVER,
            &LoginServer::announce_server,
            ::util::MilliSecond(::config::announce_server_period_ms)
        );
    }

    bool LoginServer::handle_handshake_packet(const ::net::MessageRequest& request, ::net::MessageResponse& response)
    {
        handle_handshake_packet(request);
        return true;
    }

    ::database::AsyncTask LoginServer::handle_handshake_packet(::net::MessageRequest request)
    {
        ::net::PacketRequest packet_request(request);
        ::net::PacketHandshake packet(packet_request.packet_data());

        protocol::PacketHandshakeResponse packet_response;
        packet_response.set_error_code(error::PACKET_RESULT_FAIL_LOGIN);
        packet_response.set_connection_key(packet_request.connection_key().raw());

        util::defer send_response = [&request, &packet_response]() {
            request.send_reply(packet_response);
        };

        { // Authenticate
            auto [err, result] = co_await ::database::CouchbaseCore::get_document(::database::CollectionPath::player_login, packet.username);
            if (err.ec() == couchbase::errc::key_value::document_exists) {
                packet_response.set_error_code(error::PACKET_RESULT_NOT_EXIST_LOGIN);
                co_return;
            }
            else if (err)
                co_return;

            auto player_login = result.content_as<::database::collection::PlayerLogin>();
            if (player_login.password != packet.password)
                co_return;

            packet_response.set_error_code(error::PACKET_HANDLE_SUCCESS);
            packet_response.set_player_type(player_login.player_type);
         //packet_response.set_player_identity(player_login.identity);
        }
        
        { // Update login session
            auto [err, result] = co_await ::database::CouchbaseCore::get_document(::database::CollectionPath::player_login_session, packet.username);
            if (err && err.ec() != couchbase::errc::key_value::document_not_found)
                co_return;

            auto login_session = err.ec() != couchbase::errc::key_value::document_not_found
                ? result.content_as<::database::collection::PlayerLoginSession>() 
                : ::database::collection::PlayerLoginSession{ .connection_key = packet_request.connection_key().raw() };

            packet_response.set_prev_connection_key(login_session.connection_key);

            std::tie(err, std::ignore) = co_await ::database::CouchbaseCore::upsert_document(::database::CollectionPath::player_login_session, packet.username, login_session);
            if (err) {
                CONSOLE_LOG(error) << "Fail to update login session(" << packet.username << ')';
            }
        }
    }

    bool LoginServer::handle_player_logout_message(const ::net::MessageRequest& request, ::net::MessageResponse& response)
    {
        protocol::PlayerLogoutRequest msg;
        if (not msg.ParseFromArray(request.begin_message(), int(request.message_size())))
            return false;

       // ::database::PlayerSession player_session(msg.username());
        return true; // player_session.revoke();
    }

    bool LoginServer::initialize(const char* router_ip, int router_port)
    {
        auto& comm = server_core.communicator();
        comm.register_server(protocol::ServerType::Router, { router_ip, router_port });

        if (not comm.load_remote_config(protocol::ServerType::Login, login::config::get_config()))
            return false;

        auto& conf = login::config::get_config();
        logging::initialize_system(conf.log().log_dir(), conf.log().log_filename());
    
        ::database::CouchbaseCore::connect_server_with_login(conf.session_database());

        return true;
    }

    void LoginServer::serve_forever(int argc, char* argv[])
    {
        // Initialization
        auto router_ip = argv[1];
        auto router_port = std::atoi(argv[2]);

        if (not initialize(router_ip, router_port))
            return;

        // Start server.
        auto& conf = login::config::get_config();
        server_core.start_network_io_service(conf.server().ip(), conf.server().port(), 1);

        while (1) {
            interval_tasks.process_tasks();

            util::sleep_ms(3000);
        }
    }

    void LoginServer::announce_server()
    {
        auto& conf = login::config::get_config();

        if (not server_core.communicator().announce_server(server_type, {
            .ip = conf.server().ip(),
            .port = conf.server().port()
            })) {
            CONSOLE_LOG(error) << "Fail to announce login server";
        }
    }
}
}