#include "pch.h"
#include <iostream>
#include <locale>
#include <winsock2.h>

#include "config/config.h"
#include "util/deferred_call.h"
#include "net/master_server.h"
#include "database/sql_statement.h"
#include "database/database_core.h"
#include "logging/error.h"

#include "net/deferred_packet.h"

#include "system_initializer.h"

int main()
{
	setup::initialize_system();

	try {
		logging::err() << "error!";

		auto server = net::MasterServer("127.0.0.1", 12345, 10000, 4);
		server.serve_forever();

		/*
		const auto& conf = config::get_config();

		auto db_core = database::DatabaseCore();
		if (not db_core.connect_with_password(conf.db)) {
			std::cout << "disconnected\n"; return 0;
		}

		database::PlayerSearchSQL stmt{ db_core.get_connection_handle() };
		
		std::cout << stmt.search("admin2") << '\n';
		std::cout << stmt.get_player_identity_number() << '\n';
		*/

		/*
		net::PacketHandshake pkt;
		pkt.id = 1;
		pkt.username.data = "1234";
		pkt.password.data = "1234";

		auto deferred_packet_stack = net::DeferredPacketStack();
		deferred_packet_stack.push<net::PacketHandshake>(net::DescriptorLevel::WorkerThread(1), pkt);

		pkt.id = 2;
		deferred_packet_stack.push<net::PacketHandshake>(net::DescriptorLevel::WorkerThread(2), pkt);

		pkt.id = 3;
		deferred_packet_stack.push<net::PacketHandshake>(net::DescriptorLevel::WorkerThread(3), pkt);
		
		auto deferred_pkt = deferred_packet_stack.pop<net::PacketHandshake>();
		std::cout << deferred_pkt->connection_descriptor;
		*/
	}
	catch (const error::Exception ex) {
		std::cout << ex.code << std::endl;
	}
}