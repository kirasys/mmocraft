#include <iostream>

#include <database/query.h>
#include <util/time_util.h>
#include <logging/logger.h>

#include "net/chat_server.h"

void initialize_system(int argc, char* argv[])
{
    net::Socket::initialize_system();
    
    // initialize config system.
    auto router_ip = argv[1];
    auto router_port = std::atoi(argv[2]);
    if (not ::config::load_remote_config(router_ip, router_port, protocol::ServerType::Chat, chat::config::get_config()))
        std::exit(0);

    auto& conf = chat::config::get_config();
    logging::initialize_system(conf.log().log_dir(), conf.log().log_filename());

    //chat_database::connect_mongodb_server("");
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " ROUTE_SERVER_IP ROUTE_SERVER_PORT";
        return 0;
    }

    initialize_system(argc, argv);
    
    chat::ChatServer chat_server;
    chat_server.serve_forever();
}