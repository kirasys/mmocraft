#include "login_server.h"

#include <config/config.h>
#include <database/query.h>
#include <net/packet.h>
#include <proto/generated/protocol.pb.h>
#include <util/common_util.h>
#include <logging/logger.h>

#include "../config/config.h"
#include "../database/query.h"

namespace
{

    std::array<login::net::LoginServer::handler_type, 0x100> message_handler_table = [] {
        std::array<login::net::LoginServer::handler_type, 0x100> arr{};
        arr[::net::MessageID::Login_PacketHandshake] = &login::net::LoginServer::handle_handshake_packet;
        return arr;
    }();
}

namespace login
{
namespace net
{
    LoginServer::LoginServer()
        : server_core{ this, &message_handler_table }
    {

    }

    bool LoginServer::handle_handshake_packet(const ::net::MessageRequest& request, ::net::MessageResponse& response)
    {
        protocol::PacketHandshakeResponse handshake_result;
        handshake_result.set_error_code(error::PACKET_RESULT_FAIL_LOGIN);

        util::defer set_response = [&response, &handshake_result] {
            response.set_message(handshake_result);
        };

        database::PlayerLoginSQL player_login;
        if (not player_login.is_valid()) {
            CONSOLE_LOG(error) << "Fail to allocate sql statement handles.";
            return false;
        }

        ::net::PacketRequest packet_request(request);
        ::net::PacketHandshake packet(packet_request.packet_data());

        if (not player_login.authenticate(packet.username, packet.password) || player_login.player_type() == game::PlayerType::INVALID) {
            LOG(error) << "Fail to authenticate user:" << packet.username;
            return false;
        }

        handshake_result.set_error_code(error::PACKET_HANDLE_SUCCESS);
        handshake_result.set_connection_key(packet_request.connection_key().raw());
        handshake_result.set_player_type(player_login.player_type());
        handshake_result.set_player_identity(player_login.player_identity());

        ::database::PlayerSession player_session(packet.username);
        if (player_session.exists())
            handshake_result.set_prev_connection_key(player_session.connection_key().raw());

        player_session.update(packet_request.connection_key(), player_login.player_type(), player_login.player_identity());
        return true;
    }

    bool LoginServer::initialize(const char* router_ip, int router_port)
    {
        auto& comm = server_core.communicator();
        comm.register_server(protocol::ServerType::Router, { router_ip, router_port });

        if (not comm.load_remote_config(protocol::ServerType::Login, login::config::get_config()))
            return false;

        auto& conf = login::config::get_config();
        logging::initialize_system(conf.log().log_dir(), conf.log().log_filename());
        
        ::database::DatabaseCore::connect_server_with_login(conf.player_database());
    
        ::database::MongoDBCore::connect_server(conf.session_database().server_address());

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
            server_core.communicator().announce_server(protocol::ServerType::Login, {
                .ip = conf.server().ip(),
                .port = conf.server().port()
            });

            util::sleep_ms(3000);
        }
    }
}
}