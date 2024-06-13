#include "pch.h"
#include "system_initializer.h"

#include <cstdlib>
#include <csignal>
#include <vector>

#include "logging/logger.h"
#include "net/socket.h"
#include "net/connection.h"

namespace
{
    std::vector<std::terminate_handler> system_terminatio_handlers;

    void termination_routine()
    {
        for (auto handler : system_terminatio_handlers)
            handler();

        std::cerr << "Unexpected fatal error occured! exiting.." << std::endl;
        std::exit(0);
    }

    void termination_routine_for_signal(int signal)
    {
        termination_routine();
    }
}

namespace setup
{
    void initialize_system()
    {
        std::set_terminate(termination_routine);

        std::signal(SIGTERM, termination_routine_for_signal);
        std::signal(SIGSEGV, termination_routine_for_signal);
        std::signal(SIGINT, termination_routine_for_signal);
        std::signal(SIGABRT, termination_routine_for_signal);

        // config system
        config::initialize_system();

        // log system
        logging::initialize_system();

        // network system
        net::Socket::initialize_system();
    }

    void add_termination_handler(std::terminate_handler handler)
    {
        system_terminatio_handlers.push_back(handler);
    }

    NetworkSystemInitialzer::NetworkSystemInitialzer()
    {
        net::Socket::initialize_system();
    }
}