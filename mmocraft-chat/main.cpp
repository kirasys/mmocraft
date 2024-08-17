#include <iostream>

#include "net/chat_server.h"

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " ROUTE_SERVER_IP ROUTE_SERVER_PORT";
        return 0;
    }
    
    net::Socket::initialize_system();

    chat::net::ChatServer chat_server;
    chat_server.serve_forever(argc, argv);
}