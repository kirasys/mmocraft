#pragma once

#include <exception>

namespace setup
{
    void initialize_system(int argc, char* argv[]);

    void add_termination_handler(std::terminate_handler handler);

    class SystemInitialzer
    {
    public:
        SystemInitialzer(void (*func)());
    };
}