#include "pch.h"
#include "system_initializer.h"

#include <cstdlib>
#include <csignal>
#include <filesystem>
#include <vector>

#include "config/config.h"
#include "proto/config.pb.h"
#include "logging/logger.h"
#include "net/socket.h"
#include "net/connection.h"

namespace fs = std::filesystem;

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

        config::initialize_system();

        logging::initialize_system();

        net::Socket::initialize_system();

        // Create working directories
        const auto& world_conf = config::get_world_config();

        if (not fs::exists(world_conf.save_dir()))
            fs::create_directories(world_conf.save_dir());
    }

    void add_termination_handler(std::terminate_handler handler)
    {
        system_terminatio_handlers.push_back(handler);
    }

    SystemInitialzer::SystemInitialzer(void (*func)())
    {
        func();
    }
}