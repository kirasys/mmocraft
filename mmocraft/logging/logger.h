#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <source_location>
#include <mutex>

#include "win/win_type.h"
#include <sql.h>
#include <sqlext.h>

#include "util/common_util.h"

#define ENABLE_FILE_LOGGING true
#define ENALBE_CONSOLE_LOGGING true

#define LOG(level) if (ENABLE_FILE_LOGGING) logging::##level()
#define CONSOLE_LOG(level) if (ENALBE_CONSOLE_LOGGING) logging::console_##level()

#define LOG_IF(level, cond) if ((cond) && ENABLE_FILE_LOGGING) logging::##level()
#define CONSOLE_LOG_IF(level, cond) if ((cond) && ENALBE_CONSOLE_LOGGING) logging::console_##level()

namespace logging
{
    enum LogLevel
    {
        Debug,
        Info,
        Warn,
        Error,
        Fatal,

        SIZE,
    };

    struct LogLevelDescriptor
    {
        const char* prefix_string = "[Info]";
        std::ostream* outstream = &std::cout;
        std::mutex flush_mutex;
    };

    LogLevel to_log_level(std::string log_level);

    void initialize_system(std::string_view log_dir, std::string_view log_filename);

    class Logger : util::NonCopyable, util::NonMovable
    {
    public:
        Logger(LogLevel, const std::source_location&);

        virtual ~Logger();
        
        template <typename T>
        void append(T&& value)
        {
            _buffer << std::forward<T>(value);
        }

        // template member function should be in the header.
        template <typename T>
        Logger& operator<<(T&& value)
        {
            _buffer << std::forward<T>(value);
            return *this;
        }

        std::string_view buffer_view() const
        {
            return _buffer.view();
        }

        void clear_buffer()
        {
            _buffer.str(std::string());
            _buffer.clear();
        }

        LogLevel log_level() const
        {
            return _level;
        }

        virtual void flush() = 0;

    private:
        void set_line_prefix(const std::source_location&);

        LogLevel _level;
        std::stringstream _buffer;
    };

    class ConsoleLogger : public Logger
    {
    public:
        ConsoleLogger(LogLevel, const std::source_location&);

        ~ConsoleLogger();

        virtual void flush() override;
    };

    class FileLogger : public Logger
    {
    public:
        FileLogger(LogLevel, const std::source_location&);

        ~FileLogger();

        virtual void flush() override;
    };

    // Console log functions
    ConsoleLogger console_debug(const std::source_location& location = std::source_location::current());

    ConsoleLogger console_info(const std::source_location& location = std::source_location::current());

    ConsoleLogger console_warn(const std::source_location& location = std::source_location::current());

    ConsoleLogger console_error(const std::source_location &location = std::source_location::current());
    
    ConsoleLogger console_fatal(const std::source_location &location = std::source_location::current());

    // File log functions
    FileLogger debug(const std::source_location& location = std::source_location::current());

    FileLogger info(const std::source_location& location = std::source_location::current());

    FileLogger warn(const std::source_location& location = std::source_location::current());

    FileLogger error(const std::source_location& location = std::source_location::current());
    
    FileLogger fatal(const std::source_location& location = std::source_location::current());

    void logging_sql_error(SQLSMALLINT handle_type, SQLHANDLE handle, RETCODE error_code);
}