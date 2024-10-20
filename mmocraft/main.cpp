#include "pch.h"
#include <iostream>
#include <locale>

#include "config/config.h"
#include "util/deferred_call.h"
#include "net/game_server.h"
#include "database/query.h"
#include "logging/error.h"
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

	}
	catch (const error::ErrorCode code) {
		std::cout << code << std::endl;
	}
}