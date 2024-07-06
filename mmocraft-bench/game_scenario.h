#pragma once

#include "client_bot.h"
#include <queue>

namespace bench
{
    class GameScenario
    {
    public:
        GameScenario(io::IoService&, std::size_t);

        void reset();

        auto get_clients() -> std::vector<std::unique_ptr<bench::ClientBot>>&;

        auto new_client() -> std::unique_ptr<bench::ClientBot>&;

        std::size_t size_of_max_clients() const {
            return max_client_size;
        }

        void connect_all();

        void ping_all();

        std::size_t size_of_connected_client() const;

        void print_client_status() const;

        virtual void start() = 0;

        void cancel()
        {
            canceled = true;
        }

        bool is_canceled() const
        {
            return canceled;
        }

    private:
        io::IoService& _io_service;

        const std::size_t max_client_size;
        std::vector<std::unique_ptr<bench::ClientBot>> clients;

        bool canceled = false;
    };

    class IntensiveIoScenario : public GameScenario
    {
    public:
        IntensiveIoScenario(io::IoService& io_service, std::size_t max_client_size)
            : GameScenario{ io_service, max_client_size }
        { }

        void start() override;
    };

    class FrequentAcceptScenario : public GameScenario
    {
    public:
        FrequentAcceptScenario(io::IoService& io_service, std::size_t max_client_size)
            : GameScenario{ io_service, max_client_size }
        { }

        void start() override;

    private:
        std::queue<std::unique_ptr<bench::ClientBot>> disconnected_clients;
    };

    class RandomBlockScenario : public GameScenario
    {
    public:
        RandomBlockScenario(io::IoService& io_service, std::size_t max_client_size, util::Coordinate3D a_map_size)
            : GameScenario{ io_service, max_client_size }
            , map_size{ a_map_size }
        { }

        void start() override;

    private:
        util::Coordinate3D map_size;
    };
}