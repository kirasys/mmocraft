#pragma once

#include <exception>

namespace setup
{
    void initialize_system(const char* router_ip, int router_port);

    void add_termination_handler(std::terminate_handler handler);

    class SystemInitialzer
    {
    public:
        SystemInitialzer(void (*func)());
    };
}