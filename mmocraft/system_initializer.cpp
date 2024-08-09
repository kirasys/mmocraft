#include "pch.h"
#include "system_initializer.h"

#include <cstdlib>
#include <csignal>
#include <filesystem>
#include <vector>

#include "config/config.h"
#include "logging/logger.h"
#include "net/socket.h"
#include "net/connection.h"
#include "database/query.h"

namespace fs = std::filesystem;

namespace
{
    constexpr const char* config_dir = "config";
    constexpr const char* config_filename = "config.json";

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
    void initialize_system(int argc, char* argv[])
    {
        std::set_terminate(termination_routine);

        std::signal(SIGTERM, termination_routine_for_signal);
        std::signal(SIGSEGV, termination_routine_for_signal);
        std::signal(SIGINT, termination_routine_for_signal);
        std::signal(SIGABRT, termination_routine_for_signal);

        net::Socket::initialize_system();

        auto router_ip = argv[1];
        auto router_port = std::atoi(argv[2]);

        if (not config::load_remote_config(router_ip, router_port, protocol::ServerType::Frontend, config::get_config()))
            return;

        auto& conf = config::get_config();
        logging::initialize_system(conf.log().log_dir(), conf.log().log_filename());

        database::initialize_system();

        // Create working directories
        if (not fs::exists(conf.world().save_dir()))
            fs::create_directories(conf.world().save_dir());
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