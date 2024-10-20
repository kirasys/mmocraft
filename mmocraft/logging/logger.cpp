#include "pch.h"
#include "logger.h"

#include <map>
#include <filesystem>
#include <string_view>

#include "logging/error.h"
#include "system_initializer.h"

namespace fs = std::filesystem;

namespace
{
    std::ofstream general_log_stream;
    std::ofstream error_log_stream;

    logging::LogLevelDescriptor log_level_descriptors[logging::log_level::value::count];
}

namespace logging
{
    log_level::value to_log_level(std::string log_level)
    {
        static const std::map<std::string, log_level::value> log_level_map = {
            {"DEBUG", log_level::debug},
            {"INFO", log_level::info},
            {"WARN", log_level::warn},
            {"ERROR", log_level::error},
            {"FATAL", log_level::fatal},
        };
        
        if (log_level_map.find(log_level) == log_level_map.end())
            return log_level::info; // Default

        return log_level_map.at(log_level);
    }

    void initialize_system(std::string_view log_dir, std::string_view log_filename)
    {
        setlocale(LC_ALL, ""); // user-default ANSI code page obtained from the operating system

        // Initialize log level descriptors.
        log_level_descriptors[log_level::debug].prefix_string = "[Debug]";
        log_level_descriptors[log_level::info].prefix_string = "[Info]";
        log_level_descriptors[log_level::warn].prefix_string = "[Warn]";
        log_level_descriptors[log_level::error].prefix_string = "[Error]";
        log_level_descriptors[log_level::fatal].prefix_string = "[Fatal]";

        log_level_descriptors[log_level::debug].outstream = &general_log_stream;
        log_level_descriptors[log_level::info].outstream = &general_log_stream;
        log_level_descriptors[log_level::warn].outstream = &general_log_stream;
        log_level_descriptors[log_level::error].outstream = &error_log_stream;
        log_level_descriptors[log_level::fatal].outstream = &error_log_stream;

        // Open file stream for logging.
        if (not fs::exists(log_dir))
            fs::create_directories(log_dir);

        {
            fs::path general_log_path(log_dir);
            general_log_path /= log_filename;
            general_log_stream.open(general_log_path, std::ofstream::out);
            if (not general_log_stream.is_open())
                CONSOLE_LOG(fatal) << "Fail to open file: " << general_log_path;
        }

        {
            fs::path error_log_path(log_dir);
            error_log_path /= "error_" + std::string(log_filename);
            error_log_stream.open(error_log_path, std::ofstream::out);
            if (not error_log_stream.is_open())
                CONSOLE_LOG(fatal) << "Fail to open file: " << error_log_path;
        }

        setup::add_termination_handler([]() {
            general_log_stream.close();
            error_log_stream.close();
        });
    }

    /*  Logger Class */

    Logger::Logger(log_level::value level, const std::source_location &location)
        : _level{ level }
    {
        set_line_prefix(location);
    }

    Logger::~Logger()
    {
        if (_level == log_level::fatal)
            std::exit(0);
    }

    void Logger::set_line_prefix(const std::source_location& location)
    {
        _buffer << log_level_descriptors[_level].prefix_string << ' '
            << std::filesystem::path(location.file_name()).filename() << '('
            << location.line() << ':'
            << location.column() << ") : ";
    }

    ConsoleLogger::ConsoleLogger(log_level::value level, const std::source_location& location)
        : Logger{level, location}
    { }

    ConsoleLogger::~ConsoleLogger()
    {
        flush();
    }

    void ConsoleLogger::flush()
    {
        std::cout << buffer_view() << std::endl;

        clear_buffer();
    }

    FileLogger::FileLogger(log_level::value level, const std::source_location& location)
        : Logger{ level, location }
    { }

    FileLogger::~FileLogger()
    {
        flush();
    }

    void FileLogger::flush()
    {
        {
            const std::lock_guard<std::mutex> lock(log_level_descriptors[log_level()].flush_mutex);
            *log_level_descriptors[log_level()].outstream << buffer_view() << std::endl;
        }

        clear_buffer();
    }

    ConsoleLogger console_debug(const std::source_location& location) {
        return { log_level::debug, location };
    }

    ConsoleLogger console_info(const std::source_location& location) {
        return { log_level::info, location };
    }

    ConsoleLogger console_warn(const std::source_location& location) {
        return { log_level::warn, location };
    }

    ConsoleLogger console_error(const std::source_location &location) {
        return { log_level::error, location };
    }

    ConsoleLogger console_fatal(const std::source_location& location) {
        return { log_level::fatal, location };
    }

    FileLogger debug(const std::source_location& location) {
        return { log_level::debug, location };
    }

    FileLogger info(const std::source_location& location) {
        return { log_level::info, location };
    }

    FileLogger warn(const std::source_location& location) {
        return { log_level::warn, location };
    }

    FileLogger error(const std::source_location& location) {
        return { log_level::error, location };
    }

    FileLogger fatal(const std::source_location& location) {
        return { log_level::fatal, location };
    }

    void logging_sql_error(SQLSMALLINT handle_type, SQLHANDLE handle, RETCODE error_code)
    {
        CONSOLE_LOG(error) << "SQL error_code: " << error_code;

        if (error_code == SQL_SUCCESS_WITH_INFO || error_code == SQL_ERROR) {
            SQLINTEGER native_error_code;
            WCHAR error_message[1024];
            WCHAR sql_state[SQL_SQLSTATE_SIZE + 1];

            for (SQLSMALLINT i = 1;
                ::SQLGetDiagRec(handle_type,
                    handle,
                    i,
                    sql_state,
                    &native_error_code,
                    error_message,
                    (SQLSMALLINT)(sizeof(error_message) / sizeof(*error_message)),
                    (SQLSMALLINT*)NULL) == SQL_SUCCESS;
                i++)
            {
                std::wcerr << error_message << L'\n';
                //fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, native_error);
            }
        }
    }
}