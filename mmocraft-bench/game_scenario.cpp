#include "game_scenario.h"

#include "arguments.h"
#include "util/time_util.h"

namespace bench
{
    GameScenario::GameScenario(io::IoService& io_service, std::size_t client_size)
        : _io_service(io_service)
        , max_client_size{ client_size }
    {
        reset();
        connect_all();
    }

    void GameScenario::reset()
    {
        clients.clear();
        clients.reserve(max_client_size);
        for (std::size_t i = 0; i < max_client_size; i++)
            new_client();
    }

    auto GameScenario::get_clients() -> std::vector<std::unique_ptr<bench::ClientBot>>&
    {
        return clients;
    }

    auto GameScenario::new_client() -> std::unique_ptr<bench::ClientBot>&
    {
        return clients.emplace_back(new ClientBot(_io_service));
    }

    void GameScenario::connect_all()
    {
        auto& args = bench::get_args();
        for (std::size_t i = 0; i < max_client_size; i++) {
            clients[i]->connect(args.ip, args.port);
        }
    }

    void GameScenario::ping_all()
    {
        auto& args = bench::get_args();

        for (std::size_t i = 0; i < size_of_max_clients(); i++) {
            auto state = clients[i]->state();
            if (state == ClientState::Connected)
                clients[i]->send_ping();
        }
    }

    std::size_t GameScenario::size_of_connected_client() const
    {
        return std::count_if(clients.begin(), clients.end(), [](auto& client) {
            return client->state() >= ClientState::Connected;
            });
    }

    void GameScenario::print_client_status() const
    {
        std::cout << "Connected clients: " << size_of_connected_client();
        std::cout << '\n';
    }

    void IntensiveIoScenario::start()
    {
        auto& args = bench::get_args();

        while (not is_canceled()) {
            ping_all();
            if (args.tick_interval) util::sleep_ms(args.tick_interval);
        }
    }

    void FrequentAcceptScenario::start()
    {
        auto& args = bench::get_args();

        while (not is_canceled()) {
            ping_all();

            if (args.tick_interval) util::sleep_ms(args.tick_interval);

            auto& clients = GameScenario::get_clients();
            for (std::size_t i = 0; i < clients.size(); i++) {
                if (clients[i]->state() == ClientState::Error || clients[i]->state() == Connected) {
                    clients[i]->disconnect();
                    disconnected_clients.emplace(std::move(clients[i]));
                    std::swap(clients[i], clients.back());
                    clients.pop_back();
                }
            }

            while (not disconnected_clients.empty() && disconnected_clients.front()->is_safe_delete()) {
                disconnected_clients.pop();
            }

            while (clients.size() < size_of_max_clients()) {
                auto& client = GameScenario::new_client();
                client->connect(args.ip, args.port);
            }
        }
    }

    void RandomBlockScenario::start()
    {
        auto& args = bench::get_args();
        auto& clients = GameScenario::get_clients();
        
        while (not is_canceled()) {
            for (auto& client : clients) {
                switch (client->state()) {
                case ClientState::Connected:
                {
                    client->send_handshake();
                    client->set_state(ClientState::Handshake_Requested);
                }
                break;
                case ClientState::Level_Initialized:
                {
                    client->send_random_block(map_size);
                }
                break;
                }
            }

            if (args.tick_interval) util::sleep_ms(args.tick_interval);
        }
    }
}