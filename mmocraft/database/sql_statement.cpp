#include "pch.h"
#include "sql_statement.h"

#include "logging/logger.h"

#define CHECK_DB_SUCCESS(ret) \
    if ((ret) != SQL_SUCCESS && (ret) != SQL_SUCCESS_WITH_INFO) \
        { logging_current_statement_error(ret); return false; }

#define CHECK_DB_STRONG_SUCCESS(ret) \
    if ((ret) != SQL_SUCCESS) \
        { logging_current_statement_error(ret); return false; } 

namespace database
{
    SQLStatement::SQLStatement(SQLHDBC a_connection_handle)
    {
        if (::SQLAllocHandle(SQL_HANDLE_STMT, a_connection_handle, &statement_handle) == SQL_ERROR)
            return;
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
        logging::logging_sql_error(SQL_HANDLE_STMT, statement_handle, error_code);
    }

    bool SQLStatement::prepare(std::string_view query)
    {
        auto ret = ::SQLPrepareA(statement_handle, (SQLCHAR*)query.data(), SQLINTEGER(query.size()));
        CHECK_DB_STRONG_SUCCESS(ret);
        return true;
    }

    bool SQLStatement::execute()
    {
        auto ret = ::SQLExecute(statement_handle);
        CHECK_DB_SUCCESS(ret);
        return true;
    }

    bool SQLStatement::fetch()
    {
        auto ret = ::SQLFetch(statement_handle);
        if (ret == SQL_NO_DATA) return false;

        CHECK_DB_SUCCESS(ret);
        return true;
    }

    bool SQLStatement::close_cursor()
    {
        auto ret = ::SQLCloseCursor(statement_handle);
        CHECK_DB_SUCCESS(ret);
        return true;
    }

    bool SQLStatement::inbound_bool_parameter(SQLUSMALLINT parameter_number, SQLCHAR& value)
    {
        auto ret = ::SQLBindParameter(statement_handle,
            parameter_number,
            SQL_PARAM_INPUT,
            SQL_C_BIT, SQL_BIT,
            /*ColumnSize=*/ 0,
            /*DecimalDigits=*/ 0,
            /*ParameterValuePtr=*/ &value,
            /*BufferLength=*/ 0,
            /*StrLen_or_IndPtr=*/ NULL);

        CHECK_DB_STRONG_SUCCESS(ret);

        return true;
    }

    bool SQLStatement::inbound_int32_parameter(SQLUSMALLINT parameter_number, SQLINTEGER& value)
    {
        auto ret = ::SQLBindParameter(statement_handle,
            parameter_number,
            SQL_PARAM_INPUT,
            SQL_C_SLONG, SQL_INTEGER,
            /*ColumnSize=*/ 0,
            /*DecimalDigits=*/ 0,
            /*ParameterValuePtr=*/ &value,
            /*BufferLength=*/ 0,
            /*StrLen_or_IndPtr=*/ NULL);

        CHECK_DB_STRONG_SUCCESS(ret);

        return true;
    }

    bool SQLStatement::inbound_int64_parameter(SQLUSMALLINT parameter_number, SQLBIGINT& value)
    {
        auto ret = ::SQLBindParameter(statement_handle,
            parameter_number,
            SQL_PARAM_INPUT,
            SQL_C_SBIGINT, SQL_BIGINT,
            /*ColumnSize=*/ 0,
            /*DecimalDigits=*/ 0,
            /*ParameterValuePtr=*/ &value,
            /*BufferLength=*/ 0,
            /*StrLen_or_IndPtr=*/ NULL);

        CHECK_DB_STRONG_SUCCESS(ret);

        return true;
    }

    bool SQLStatement::inbound_uint32_parameter(SQLUSMALLINT parameter_number, SQLUINTEGER& value)
    {
        auto ret = ::SQLBindParameter(statement_handle,
            parameter_number,
            SQL_PARAM_INPUT,
            SQL_C_ULONG, SQL_INTEGER,
            /*ColumnSize=*/ 0,
            /*DecimalDigits=*/ 0,
            /*ParameterValuePtr=*/ &value,
            /*BufferLength=*/ 0,
            /*StrLen_or_IndPtr=*/ NULL);

        CHECK_DB_STRONG_SUCCESS(ret);

        return true;
    }

    bool SQLStatement::inbound_uint64_parameter(SQLUSMALLINT parameter_number, SQLUBIGINT& value)
    {
        auto ret = ::SQLBindParameter(statement_handle,
            parameter_number,
            SQL_PARAM_INPUT,
            SQL_C_UBIGINT, SQL_BIGINT,
            /*ColumnSize=*/ 0,
            /*DecimalDigits=*/ 0,
            /*ParameterValuePtr=*/ &value,
            /*BufferLength=*/ 0,
            /*StrLen_or_IndPtr=*/ NULL);

        CHECK_DB_STRONG_SUCCESS(ret);

        return true;
    }

    bool SQLStatement::inbound_chars_parameter(SQLUSMALLINT parameter_number, const char* buf, SQLLEN buf_size, SQLLEN& data_size)
    {
        auto ret = ::SQLBindParameter(statement_handle,
            parameter_number,
            SQL_PARAM_INPUT,
            SQL_C_CHAR, SQL_CHAR,
            /*ColumnSize=*/ buf_size,
            /*DecimalDigits=*/ 0,
            /*ParameterValuePtr=*/ (SQLCHAR*)buf,
            /*BufferLength=*/ 0,
            /*StrLen_or_IndPtr=*/ &data_size);

        CHECK_DB_STRONG_SUCCESS(ret);

        return true;
    }

    bool SQLStatement::inbound_null_terminated_string_parameter(SQLUSMALLINT parameter_number, const char* cstr, SQLLEN buf_size)
    {
        auto ret = ::SQLBindParameter(statement_handle,
            parameter_number,
            SQL_PARAM_INPUT,
            SQL_C_CHAR, SQL_CHAR,
            /*ColumnSize=*/ buf_size,
            /*DecimalDigits=*/ 0,
            /*ParameterValuePtr=*/ (SQLCHAR*)cstr,
            /*BufferLength=*/ 0,
            /*StrLen_or_IndPtr=*/ NULL);

        CHECK_DB_STRONG_SUCCESS(ret);

        return true;
    }

    bool SQLStatement::outbound_bool_column(SQLUSMALLINT column_number, SQLCHAR& value)
    {
        auto ret = ::SQLBindCol(statement_handle, column_number, SQL_C_BIT, &value, 0, NULL);
        CHECK_DB_STRONG_SUCCESS(ret);
        return true;
    }

    bool SQLStatement::outbound_int32_column(SQLUSMALLINT column_number, SQLINTEGER& value)
    {
        auto ret = ::SQLBindCol(statement_handle, column_number, SQL_C_SLONG, &value, 0, NULL);
        CHECK_DB_STRONG_SUCCESS(ret);
        return true;
    }

    bool SQLStatement::outbound_int64_column(SQLUSMALLINT column_number, SQLBIGINT& value)
    {
        auto ret = ::SQLBindCol(statement_handle, column_number, SQL_C_SBIGINT, &value, 0, NULL);
        CHECK_DB_STRONG_SUCCESS(ret);
        return true;
    }

    bool SQLStatement::outbound_uint32_column(SQLUSMALLINT column_number, SQLUINTEGER& value)
    {
        auto ret = ::SQLBindCol(statement_handle, column_number, SQL_C_ULONG, &value, 0, NULL);
        CHECK_DB_STRONG_SUCCESS(ret);
        return true;
    }

    bool SQLStatement::outbound_uint64_column(SQLUSMALLINT column_number, SQLUBIGINT& value)
    {
        auto ret = ::SQLBindCol(statement_handle, column_number, SQL_C_UBIGINT, &value, 0, NULL);
        CHECK_DB_STRONG_SUCCESS(ret);
        return true;
    }
}