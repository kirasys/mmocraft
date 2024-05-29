#include "pch.h"
#include "sql_statement.h"

#include <string.h>

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
        CHECK_DB_SUCCESS(ret);
        return true;
    }

    bool SQLStatement::close_cursor()
    {
        auto ret = ::SQLCloseCursor(statement_handle);
        CHECK_DB_SUCCESS(ret);
        return true;
    }

    bool SQLStatement::inbound_integer_parameter(SQLUSMALLINT parameter_number, SQLINTEGER& value)
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

    bool SQLStatement::outbound_integer_column(SQLUSMALLINT column_number, SQLINTEGER& value)
    {
        auto ret = ::SQLBindCol(statement_handle, column_number, SQL_C_SLONG, &value, 0, NULL);
        CHECK_DB_STRONG_SUCCESS(ret);
        return true;
    }

    bool SQLStatement::outbound_unsigned_integer_column(SQLUSMALLINT column_number, SQLUINTEGER& value)
    {
        auto ret = ::SQLBindCol(statement_handle, column_number, SQL_C_ULONG, &value, 0, NULL);
        CHECK_DB_STRONG_SUCCESS(ret);
        return true;
    }

    PlayerAuthSQL::PlayerAuthSQL(SQLHDBC a_connection_handle)
        : SQLStatement{a_connection_handle}
    {
        // bind input parameters.
        this->inbound_null_terminated_string_parameter(1, _username, sizeof(_username));
        this->inbound_null_terminated_string_parameter(2, _password, sizeof(_password));

        // bind output parameters.
        this->outbound_unsigned_integer_column(1, selected_player_count);
    }

    bool PlayerAuthSQL::authenticate(const char* a_username, const char* a_password)
    {
        this->prepare(sql_select_player_by_username_and_password);

        ::strcpy_s(_username, sizeof(_username), a_username);
        ::strcpy_s(_password, sizeof(_password), a_password);

        if (this->execute()) {
            util::defer clear_cursor = [this] { this->close_cursor(); };
            return this->fetch() && selected_player_count == 1;
        }

        return false;
    }

    bool PlayerAuthSQL::is_exist_username(const char* a_username)
    {
        this->prepare(sql_select_player_by_username);

        ::strcpy_s(_username, sizeof(_username), a_username);

        if (this->execute()) {
            util::defer clear_cursor = [this] { this->close_cursor(); };
            return this->fetch() && selected_player_count == 1;
        }

        return false;
    }
}