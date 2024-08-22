#include "net/login_server.h"

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " ROUTE_SERVER_IP ROUTE_SERVER_PORT";
        return 0;
    }

    net::Socket::initialize_system();

    login::net::LoginServer login_server;
    login_server.serve_forever(argc, argv);
}