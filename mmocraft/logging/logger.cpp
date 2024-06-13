#include "pch.h"
#include "logger.h"

#include <map>
#include <mutex>
#include <filesystem>
#include <string_view>

#include "config/config.h"
#include "proto/config.pb.h"
#include "logging/error.h"
#include "system_initializer.h"

namespace
{
    std::ofstream system_log_file_stream;
    std::mutex system_log_mutex;
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
        const auto& conf = config::get_config();

        setlocale(LC_ALL, ""); // user-default ANSI code page obtained from the operating system

        system_log_file_stream.open(conf.log_file_path(), std::ofstream::out);
        if (not system_log_file_stream.is_open())
            CONSOLE_LOG(fatal) << "Fail to open file: " << conf.log_file_path();

        setup::add_termination_handler([]() {
            system_log_file_stream.close();
        });
    }

    /*  LogStream Class */

    Logger::Logger
        (bool is_fatal, std::ostream& os, const std::source_location &location)
        : _is_fatal{ is_fatal }
        , _output_stream{ os }
    {
        set_line_prefix(location);
    }

    Logger::~Logger()
    {
        if (_is_fatal)
            std::exit(0);
    }

    void Logger::flush()
    {
        {
            const std::lock_guard<std::mutex> lock(system_log_mutex);
            _output_stream << _buffer.view() << std::endl;
        }

        _buffer.str(std::string());
        _buffer.clear();
    }

    void Logger::set_line_prefix(const std::source_location& location)
    {
        _buffer << std::filesystem::path(location.file_name()).filename() << '('
            << location.line() << ':'
            << location.column() << ") : ";
    }

    LogStream::LogStream
        (bool is_fatal, std::ostream& os, const std::source_location& location)
        : logger{ is_fatal, os, location }
    {
        
    }

    LogStream::~LogStream()
    {
        logger.flush();
    }

    LogStream console_error(const std::source_location &location) {
        return { false, std::cerr, location };
    }

    LogStream console_fatal(const std::source_location& location) {
        return { true, std::cerr, location };
    }

    LogStream error(const std::source_location& location) {
        return { false, system_log_file_stream, location };
    }

    LogStream fatal(const std::source_location& location) {
        return { true, system_log_file_stream, location };
    }

    void logging_sql_error(SQLSMALLINT handle_type, SQLHANDLE handle, RETCODE error_code)
    {
        std::cout << "error_code: " << error_code << '\n';
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