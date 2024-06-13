#include "pch.h"
#include "database_core.h"

#include <iostream>
#include <format>
#include <string>

#include "logging/error.h"
#include "proto/config.pb.h"

#define CHECK_DB_SUCCESS(ret) \
    if ((ret) != SQL_SUCCESS && (ret) != SQL_SUCCESS_WITH_INFO) \
        { logging_current_connection_error(ret); return false; }

#define CHECK_DB_STRONG_SUCCESS(ret) \
    if ((ret) != SQL_SUCCESS) \
        { logging_current_connection_error(ret); return false; }

namespace database
{
    DatabaseCore::DatabaseCore()
    {
        if (::SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &environment_handle) == SQL_ERROR)
            throw error::DATABASE_ALLOC_ENVIRONMENT_HANDLE;

        if (::SQLSetEnvAttr(environment_handle, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0) == SQL_ERROR)
            throw error::DATABASE_SET_ATTRIBUTE_VERSION;

        if (::SQLAllocHandle(SQL_HANDLE_DBC, environment_handle, &connection_handle) == SQL_ERROR)
            throw error::DATABASE_ALLOC_CONNECTION_HANDLE;

        _state = State::Initialized;
    }

    DatabaseCore::~DatabaseCore()
    {
        disconnect();

        if (environment_handle)
            ::SQLFreeHandle(SQL_HANDLE_ENV, environment_handle);
    }

    bool DatabaseCore::connect(std::string_view connection_string)
    {
        if (status() == Connected) return false;

        auto ret = ::SQLDriverConnectA(connection_handle,
            NULL,
            (SQLCHAR*)connection_string.data(),
            (SQLSMALLINT)connection_string.size(),
            NULL, 0, NULL, SQL_DRIVER_COMPLETE);

        CHECK_DB_SUCCESS(ret);

        _state = State::Connected;
        return true;
    }

    bool DatabaseCore::connect_with_password(const config::Configuration_Database& conf)
    {
        const std::string connection_string{
            std::format("Driver={{{}}}; Server={}; Database={}; UID={}; PWD={}; Trusted_Connection=yes",
                        conf.driver_name(),
                        conf.server_address(),
                        conf.database_name(),
                        conf.userid(),
                        conf.password())
        };

        return connect(connection_string);
    }

    void DatabaseCore::disconnect()
    {
        if (status() == Connected) {
            ::SQLDisconnect(connection_handle);
            ::SQLFreeHandle(SQL_HANDLE_DBC, connection_handle);
            _state = State::Disconnected;
        }
    }

    void DatabaseCore::logging_current_connection_error(RETCODE error_code) const
    {
        logging::logging_sql_error(SQL_HANDLE_DBC, connection_handle, error_code);
    }
}