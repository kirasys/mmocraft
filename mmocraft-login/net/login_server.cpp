#include "login_server.h"

#include <config/config.h>
#include <database/query.h>
#include <net/packet.h>
#include <proto/generated/protocol.pb.h>
#include <util/common_util.h>
#include <logging/logger.h>

#include "../config/config.h"

namespace login
{
namespace net
{
    LoginServer::LoginServer()
        : server_core{ *this }
        , interval_tasks{ this }
    {
        interval_tasks.schedule(
            ::util::interval_task_tag_id::announce_server,
            &LoginServer::announce_server,
            ::util::MilliSecond(::config::task::announce_server_period)
        );
    }

    bool LoginServer::handle_message(::net::MessageRequest& request)
    {
        switch (request.message_id()) {
        case ::net::message_id::packet_handshake:
            handle_handshake_packet(request);
            return true;
        case ::net::message_id::player_logout:
            handle_player_logout_message(request);
            return true;
        default:
            return false;
        }
    }

    database::AsyncTask LoginServer::handle_handshake_packet(::net::MessageRequest request)
    {
        ::net::PacketRequest packet_request(request);
        ::net::PacketHandshake packet(packet_request.packet_data());

        protocol::PacketHandshakeResponse packet_response;
        packet_response.set_error_code(error::code::packet::player_login_fail);
        packet_response.set_connection_key(packet_request.connection_key().raw());

        util::defer send_response = [&request, &packet_response]() {
            request.send_reply(packet_response);
        };

        { // Authenticate
            auto [err, result] = co_await database::CouchbaseCore::get_document(database::CollectionPath::player_login, packet.username);
            if (err.ec() == couchbase::errc::key_value::document_not_found) {
                packet_response.set_error_code(error::code::packet::player_not_exist);
                co_return;
            }
            else if (err)
                co_return;

            auto player_login = result.content_as<database::collection::PlayerLogin>();
            if (player_login.password != packet.password)
                co_return;

            packet_response.set_error_code(error::code::success);
            packet_response.set_player_type(player_login.player_type);
            packet_response.set_player_uuid(player_login.identity);
        }
        
        { // Update login session
            auto [err, result] = co_await database::CouchbaseCore::get_document(::database::CollectionPath::player_login_session, packet.username);
            if (err && err.ec() != couchbase::errc::key_value::document_not_found)
                co_return;

            auto login_session = err.ec() != couchbase::errc::key_value::document_not_found
                ? result.content_as<database::collection::PlayerLoginSession>() 
                : database::collection::PlayerLoginSession{ .connection_key = packet_request.connection_key().raw() };

            packet_response.set_prev_connection_key(login_session.connection_key);

            std::tie(err, std::ignore) = co_await database::CouchbaseCore::upsert_document(database::CollectionPath::player_login_session, packet.username, login_session);
            if (err) {
                CONSOLE_LOG(error) << "Fail to update login session(" << packet.username << ')';
            }
        }
    }

    database::AsyncTask LoginServer::handle_player_logout_message(::net::MessageRequest& request)
    {
        protocol::PlayerLogoutRequest msg;
        if (not request.parse_message(msg))
            co_return;

       // ::database::PlayerSession player_session(msg.username());
       // player_session.revoke();
    }

    bool LoginServer::initialize(const char* router_ip, int router_port)
    {
        auto& comm = server_core.communicator();
        comm.register_server(protocol::server_type_id::router, { router_ip, router_port });

        if (not comm.load_remote_config(protocol::server_type_id::login, login::config::get_config()))
            return false;

        auto& conf = login::config::get_config();
        logging::initialize_system(conf.log().log_dir(), conf.log().log_filename());
    
        database::CouchbaseCore::connect_server_with_login(conf.session_database());

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