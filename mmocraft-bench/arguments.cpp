#include "arguments.h"

namespace bench
{
    void parse_arguments(int argc, char* args[])
    {
        argparse::ArgumentParser program("mmocraft_benchmark");
        program.add_argument("ip")
            .required()
            .help("IP address of the game server");
        program.add_argument("port")
            .required()
            .help("Port number of the game server")
            .scan<'i', int>();
        program.add_argument("--max_client")
            .required()
            .help("Maximum number of clients")
            .scan<'i', int>();
        program.add_argument("--worker_thread")
            .default_value(1)
            .help("Number of worker threads")
            .scan<'i', int>();
        program.add_argument("--event_thread")
            .default_value(1)
            .help("Number of I/O event threads")
            .scan<'i', int>();
        program.add_argument("--interval")
            .default_value(0)
            .help("Thread tick cooltime (ms)")
            .scan<'i', int>();

        try {
            program.parse_args(argc, args);
        }
        catch (const std::exception& err) {
            std::cerr << err.what() << std::endl;
            std::cerr << program;
            std::exit(1);
        }

        auto& Args = get_args();
        Args.ip = program.get<std::string>("ip");
        Args.port = program.get<int>("port");
        Args.max_client = program.get<int>("--max_client");
        Args.num_of_worker_thread = program.get<int>("--worker_thread");
        Args.num_of_event_worker_thread = program.get<int>("--event_thread");
        Args.tick_interval = std::size_t(program.get<int>("--interval"));
    }

    Arguments& get_args()
    {
        static Arguments args;
        return args;
    }
}