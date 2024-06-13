#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <source_location>

#include <sql.h>
#include <sqlext.h>

#include "util/common_util.h"

#define ENABLE_FILE_LOGGING true

#define LOG(level) if (ENABLE_FILE_LOGGING) logging::##level()
#define CONSOLE_LOG(level) logging::console_##level()

#define LOG_IF(level, cond) if ((cond) && ENABLE_FILE_LOGGING) logging::##level()
#define CONSOLE_LOG_IF(level, cond) if ((cond)) logging::console_##level()

namespace logging
{
    enum LogLevel
    {
        Debug,
        Info,
        Warn,
        Error,
        Fatal,
    };

    LogLevel to_log_level(std::string log_level);

    void initialize_system();

    class Logger : util::NonCopyable, util::NonMovable
    {
    public:
        Logger(bool is_fatal, std::ostream&, const std::source_location&);

        ~Logger();
        
        template <typename T>
        void append(T&& value)
        {
            _buffer << std::forward<T>(value);
        }

        void flush();

    private:
        void set_line_prefix(const std::source_location&);

        bool _is_fatal;
        std::ostream& _output_stream;
        std::stringstream _buffer;
    };

    class LogStream
    {
    public:
        LogStream(bool is_fatal, std::ostream&, const std::source_location&);

        ~LogStream();

        // template member function should be in the header.
        template <typename T>
        LogStream& operator<<(T&& value)
        {
            logger.append(std::forward<T>(value));
            return *this;
        }

    private:
        Logger logger;
    };

    // Console log functions
    LogStream console_error(const std::source_location &location = std::source_location::current());
    
    LogStream console_fatal(const std::source_location &location = std::source_location::current());


    // File log functions
    LogStream error(const std::source_location& location = std::source_location::current());
    
    LogStream fatal(const std::source_location& location = std::source_location::current());

    void logging_sql_error(SQLSMALLINT handle_type, SQLHANDLE handle, RETCODE error_code);
}