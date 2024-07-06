#include "mock_database.h"
#include "database/sql_statement.h"

namespace
{
    test::MockDatabase* mock = nullptr;
}

test::MockDatabase::MockDatabase()
{
    mock = this;
}

test::MockDatabase::~MockDatabase()
{
    mock = nullptr;
}

namespace database
{
    SQLStatement::SQLStatement(SQLHDBC a_connection_handle)
    {
        
    }

    SQLStatement::~SQLStatement()
    {
        close();
    }

    void SQLStatement::close()
    {
        if (SQL_NULL_HSTMT != SQL_NULL_HSTMT)
            ::SQLFreeHandle(SQL_HANDLE_STMT, statement_handle);
        statement_handle = SQL_NULL_HSTMT;
    }

    void SQLStatement::logging_current_statement_error(RETCODE error_code) const
    {
        
    }

    bool SQLStatement::prepare(std::string_view query)
    {
        return true;
    }

    bool SQLStatement::execute()
    {
        return true;
    }

    bool SQLStatement::fetch()
    {
        if (mock)
            return mock->fetch();
        return true;
    }

    bool SQLStatement::close_cursor()
    {
        return true;
    }

    bool SQLStatement::inbound_int32_parameter(SQLUSMALLINT parameter_number, SQLINTEGER& value)
    {
        return true;
    }

    bool SQLStatement::inbound_chars_parameter(SQLUSMALLINT parameter_number, const char* buf, SQLLEN buf_size, SQLLEN& data_size)
    {
        return true;
    }

    bool SQLStatement::inbound_null_terminated_string_parameter(SQLUSMALLINT parameter_number, const char* cstr, SQLLEN buf_size)
    {
        return true;
    }

    bool SQLStatement::outbound_integer_column(SQLUSMALLINT column_number, SQLINTEGER& value)
    {
        if (mock)
            mock->outbound_integer_column(value);
        return true;
    }

    bool SQLStatement::outbound_unsigned_integer_column(SQLUSMALLINT column_number, SQLUINTEGER& value)
    {
        if (mock)
            mock->outbound_integer_column(value);
        return true;
    }
}