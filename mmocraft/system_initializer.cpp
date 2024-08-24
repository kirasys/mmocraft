#include "pch.h"
#include "system_initializer.h"

#include <cstdlib>
#include <csignal>
#include <vector>

#include "config/config.h"
#include "net/socket.h"
#include "net/server_communicator.h"
#include "logging/logger.h"
#include "database/database_core.h"
#include "database/mongodb_core.h"

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
    void initialize_system(const char* router_ip, int router_port)
    {
        std::set_terminate(termination_routine);

        std::signal(SIGTERM, termination_routine_for_signal);
        std::signal(SIGSEGV, termination_routine_for_signal);
        std::signal(SIGINT, termination_routine_for_signal);
        std::signal(SIGABRT, termination_routine_for_signal);

        net::Socket::initialize_system();

        // Load config from the route server.
        if (not net::ServerCommunicator::load_remote_config(router_ip, router_port, protocol::ServerType::Frontend, config::get_config()))
            return;

        auto& conf = config::get_config();
        config::set_default_configuration(*conf.mutable_system());

        logging::initialize_system(conf.log().log_dir(), conf.log().log_filename());

        database::DatabaseCore::connect_server_with_login(conf.player_database());
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