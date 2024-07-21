#include "pch.h"
#include "logger.h"

#include <map>
#include <filesystem>
#include <string_view>

#include "config/config.h"
#include "proto/config.pb.h"
#include "logging/error.h"
#include "system_initializer.h"

namespace
{
    std::ofstream general_log_stream;
    std::ofstream error_log_stream;

    logging::LogLevelDescriptor log_level_descriptors[logging::LogLevel::SIZE];
}

namespace logging
{
    LogLevel to_log_level(std::string log_level)
    {
        static const std::map<std::string, LogLevel> log_level_map = {
            {"DEBUG", LogLevel::Debug},
            {"INFO", LogLevel::Info},
            {"WARN", LogLevel::Warn},
            {"ERROR", LogLevel::Error},
            {"FATAL", LogLevel::Fatal},
        };
        
        if (log_level_map.find(log_level) == log_level_map.end())
            return LogLevel::Info; // Default

        return log_level_map.at(log_level);
    }

    void initialize_system()
    {
        const auto& log_conf = config::get_log_config();

        setlocale(LC_ALL, ""); // user-default ANSI code page obtained from the operating system

        // Initialize log level descriptors.
        log_level_descriptors[LogLevel::Debug].prefix_string = "[Debug]";
        log_level_descriptors[LogLevel::Info].prefix_string = "[Info]";
        log_level_descriptors[LogLevel::Warn].prefix_string = "[Warn]";
        log_level_descriptors[LogLevel::Error].prefix_string = "[Error]";
        log_level_descriptors[LogLevel::Fatal].prefix_string = "[Fatal]";

        log_level_descriptors[LogLevel::Debug].outstream = &general_log_stream;
        log_level_descriptors[LogLevel::Info].outstream = &general_log_stream;
        log_level_descriptors[LogLevel::Warn].outstream = &general_log_stream;
        log_level_descriptors[LogLevel::Error].outstream = &error_log_stream;
        log_level_descriptors[LogLevel::Fatal].outstream = &error_log_stream;

        // Open file stream for logging.
        general_log_stream.open(log_conf.log_file_path(), std::ofstream::out);
        if (not general_log_stream.is_open())
            CONSOLE_LOG(fatal) << "Fail to open file: " << log_conf.log_file_path();

        error_log_stream.open(log_conf.error_log_file_path(), std::ofstream::out);
        if (not error_log_stream.is_open())
            CONSOLE_LOG(fatal) << "Fail to open file: " << log_conf.error_log_file_path();

        setup::add_termination_handler([]() {
            general_log_stream.close();
            error_log_stream.close();
        });
    }

    /*  Logger Class */

    Logger::Logger(LogLevel level, const std::source_location &location)
        : _level{ level }
    {
        set_line_prefix(location);
    }

    Logger::~Logger()
    {
        if (_level == LogLevel::Fatal)
            std::exit(0);
    }

    void Logger::set_line_prefix(const std::source_location& location)
    {
        _buffer << log_level_descriptors[_level].prefix_string << ' '
            << std::filesystem::path(location.file_name()).filename() << '('
            << location.line() << ':'
            << location.column() << ") : ";
    }

    ConsoleLogger::ConsoleLogger(LogLevel level, const std::source_location& location)
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

    FileLogger::FileLogger(LogLevel level, const std::source_location& location)
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
        return { LogLevel::Debug, location };
    }

    ConsoleLogger console_info(const std::source_location& location) {
        return { LogLevel::Info, location };
    }

    ConsoleLogger console_warn(const std::source_location& location) {
        return { LogLevel::Warn, location };
    }

    ConsoleLogger console_error(const std::source_location &location) {
        return { LogLevel::Error, location };
    }

    ConsoleLogger console_fatal(const std::source_location& location) {
        return { LogLevel::Fatal, location };
    }

    FileLogger debug(const std::source_location& location) {
        return { LogLevel::Debug, location };
    }

    FileLogger info(const std::source_location& location) {
        return { LogLevel::Info, location };
    }

    FileLogger warn(const std::source_location& location) {
        return { LogLevel::Warn, location };
    }

    FileLogger error(const std::source_location& location) {
        return { LogLevel::Error, location };
    }

    FileLogger fatal(const std::source_location& location) {
        return { LogLevel::Fatal, location };
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