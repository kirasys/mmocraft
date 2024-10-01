#include "pch.h"
#include <iostream>
#include <locale>

#include "config/config.h"
#include "util/deferred_call.h"
#include "net/game_server.h"
#include "database/query.h"
#include "logging/error.h"
#include "net/deferred_packet.h"
#include "system_initializer.h"

#include <couchbase/cluster.hxx>
#include <couchbase/fmt/error.hxx>

int main(int argc, char* argv[])
{
	if (argc != 3) {
		std::cout << "Usage: " << argv[0] << " ROUTE_SERVER_IP ROUTE_SERVER_PORT";
		return 0;
	}

	auto router_ip = argv[1];
	auto router_port = std::atoi(argv[2]);

	setup::initialize_system(router_ip, router_port);

	try {

		auto& conf = config::get_config();
		auto server = net::GameServer(conf.tcp_server().max_client());
		server.serve_forever(router_ip, router_port);
		
		/*
		std::byte* data = (std::byte*)"123213123";
		auto event = new io::IoMulticastSendEvent;
		event->set_multicast_data(data, 9);
		
		net::Socket sock{ net::SocketProtocol::TCPv4Overlapped };
		sock.bind("127.0.0.1", 9003);
		sock.listen();
		*/
		/*
		io::RegisteredIO io_service{ 1000 };

		net::ConnectionEnvironment connection_env{ 1000 };
		net::PacketHandleServerStub packet_server;
		net::TcpServer tcp_server{ packet_server, connection_env, io_service };

		tcp_server.start_network_io_service("127.0.0.1", 9002, 4);
		*/
		while (true) {
			/*
			win::Socket accepted_socket = ::accept(sock.get_handle(), NULL, NULL);
			if (accepted_socket == INVALID_SOCKET)
			{
				printf_s("[DEBUG] accept: invalid socket\n");
				continue;
			}
			
			event->post_overlapped_io(accepted_socket);
			*/

			while (1) {
				//net::ConnectionIO::flush_receive(connection_env);

				util::sleep_ms(300);
			}
		}
		

		/*

		auto db_core = database::DatabaseCore();
		if (not db_core.connect_with_password(conf.db)) {
			std::cout << "disconnected\n"; return 0;
		}

		database::PlayerSearchSQL stmt{ db_core.get_connection_handle() };
		
		std::cout << stmt.search("admin2") << '\n';
		std::cout << stmt.get_player_identity_number() << '\n';
		*/
	}
	catch (const error::ErrorCode code) {
		std::cout << code << std::endl;
	}
}