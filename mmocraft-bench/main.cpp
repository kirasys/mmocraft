#include "arguments.h"
#include "client_bot.h"
#include "game_scenario.h"
#include "statistics.h"
#include "system_initializer.h"

#include "config/config.h"
#include "proto/generated/config.pb.h"

enum ScenarioType {
    IntensiveIO = 1,
    FrequentAccept = 2,
    RandomBlock = 3,
};

void print_available_scenarios()
{
    std::cout << ScenarioType::IntensiveIO << ". Intensive I/O scenario \n";
    std::cout << "\t: Sending a lot of small packets in a short time \n";
    std::cout << ScenarioType::FrequentAccept << ". Frequent accept scenario \n";
    std::cout << "\t: Connecting and Disconnecting repeatedly \n";
    std::cout << ScenarioType::RandomBlock << ". Random block scenario \n";
    std::cout << "\t: Create/Remove random blocks repeatedly \n";
    std::cout << " >> ";
}

bench::GameScenario* create_scenario(ScenarioType type, io::IoService& io_service, std::size_t max_client_size)
{
    auto& conf = config::get_config();

    switch (type)
    {
    case ScenarioType::IntensiveIO: return new bench::IntensiveIoScenario(io_service, max_client_size);
    case ScenarioType::FrequentAccept: return new bench::FrequentAcceptScenario(io_service, max_client_size);
    case ScenarioType::RandomBlock:
    {
        auto map_size = util::Coordinate3D{ conf.world().width(),
                                             conf.world().height(),
                                             conf.world().length()};
        return new bench::RandomBlockScenario(io_service, max_client_size, map_size);
    }
    default:
        std::cout << "Invalid scenario number\n";
    }

    return nullptr;
}

int main(int argc, char* args[])
{
    bench::parse_arguments(argc, args);
    setup::initialize_system("192.168.21.12", 20000);

    print_available_scenarios();
    int selection = 0;
    std::cin >> selection;

    auto& Args = bench::get_args();

    io::IoCompletionPort io_service(Args.num_of_event_worker_thread);
    for (int i=0; i < Args.num_of_event_worker_thread * 2; i++)
        io_service.spawn_event_loop_thread().detach();
    
    std::vector<bench::GameScenario*> scenarios;

    for (int i = 0; i < Args.num_of_worker_thread; i++) {
        scenarios.push_back(create_scenario(ScenarioType(selection), io_service, Args.max_client / Args.num_of_worker_thread));
        std::thread([](bench::GameScenario* game_scenario) {
            game_scenario->start();
        }, scenarios[i]).detach();
    }

    bench::start_benchmark();

    while (1) {
        std::cout << "\n[Connection status per thread]\n";
        for (int i = 0; i < Args.num_of_worker_thread; i++) {
            std::cout << "Thread #" << i + 1 << " ";
            scenarios[i]->print_client_status();
        }

        std::cout << "\n[Network I/O status]\n";
        bench::print_statistics();

        std::cout << "\n----------------------------------\n";

        util::sleep_ms(1000 * 2);
    }
}