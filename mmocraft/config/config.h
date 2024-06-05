#pragma once
#include <string>
#include <tuple>

namespace config {
    struct MegaBytes
    {
        int mb = 0;
        constexpr MegaBytes(int Mb) : mb(Mb) { }
        constexpr int to_bytes() const { return mb * 1048576; }
    };

    struct Configuration {
        bool loaded;

        struct SystemConfig
        {
            unsigned page_size = 0;
            unsigned alllocation_granularity = 0;
            unsigned num_of_processors = 0;
            MegaBytes memory_pool_size { 128 };
        } system;

        struct ServerConfig
        {
            std::string ip = "127.0.0.1";
            int port = 12345;
            unsigned max_player = 100;
            std::string server_name = "Minecraft Server";
            std::string motd = "welcome";
        } server;

        struct LogConfig
        {
            std::string level = "ERROR";
            std::string file_path = "server_log.txt";
        } log;

        struct LoginConfig
        {

        } login;

        struct DatabaseConfig
        {
            std::string driver_name;
            std::string server_address;
            std::string database;
            std::string userid;
            std::string password;
        } db;

        Configuration() :loaded(false)
        {

        }

        Configuration clone() const
        {
            return *this;
        }


        Configuration& operator=(Configuration&) = delete;
        Configuration& operator=(Configuration&&) = delete;

    private:
        // disallow implicit copy constructing.
        Configuration(const Configuration&) = default;
        Configuration(Configuration&&) = default;
    };

    bool load_config(Configuration&);

    const Configuration& get_config();

    void initialize_system();
}