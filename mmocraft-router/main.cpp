#include <cstdlib>

#include "net/route_server.h"
#include "config/config.h"

void initialization()
{
    net::Socket::initialize_system();
    router::config::initialize_system();
}

int main(int argc, char* argv[])
{
    initialization();

    router::net::RouteServer route_server;
    route_server.serve_forever();
}