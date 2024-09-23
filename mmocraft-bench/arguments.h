#pragma once

#include <config/argparse.h>

namespace bench
{
    struct Arguments {
        std::string ip;
        int port = 0;

        std::string router_ip;
        int router_port = 0;

        int max_client = 0;

        int num_of_worker_thread = 0;
        int num_of_event_worker_thread = 0;

        std::size_t tick_interval = 0;

        Arguments() = default;

        Arguments(Arguments&) = delete;
        Arguments& operator=(Arguments&) = delete;
    };

    void parse_arguments(int argc, char* args[]);

    Arguments& get_args();
}