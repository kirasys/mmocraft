#pragma once
#include <string_view>

#include <sql.h>
#include <sqlext.h>

#include "win/win_type.h"
#include "util/common_util.h"

namespace database
{
    class SQLStatement : util::NonCopyable
    {
    public:
        SQLStatement(SQLHDBC);

        virtual ~SQLStatement();

        SQLStatement(SQLStatement&& other) noexcept
            : statement_handle{ other.statement_handle }
        {
            other.statement_handle = SQL_NULL_HSTMT;
        }

        SQLStatement& operator=(SQLStatement&& other) noexcept
        {
            if (this != &other) {
                close();
                std::swap(statement_handle, other.statement_handle);
            }
        }

        void logging_current_statement_error(RETCODE) const;

        void close();

        bool inbound_bool_parameter(SQLUSMALLINT parameter_number, SQLCHAR&);

        bool inbound_int32_parameter(SQLUSMALLINT parameter_number, SQLINTEGER&);

        bool inbound_int64_parameter(SQLUSMALLINT parameter_number, SQLBIGINT&);

        bool inbound_uint32_parameter(SQLUSMALLINT parameter_number, SQLUINTEGER&);

        bool inbound_uint64_parameter(SQLUSMALLINT parameter_number, SQLUBIGINT&);

        bool inbound_chars_parameter(SQLUSMALLINT parameter_number, const char*, SQLLEN, SQLLEN&);

        bool inbound_bytes_parameter(SQLUSMALLINT parameter_number, const std::byte*, SQLLEN, SQLLEN&);

        bool inbound_null_terminated_string_parameter(SQLUSMALLINT parameter_number, const char*, SQLLEN);

        bool outbound_uint32_parameter(SQLUSMALLINT parameter_number, SQLUINTEGER&);

        bool outbound_bytes_parameter(SQLUSMALLINT column_number, std::byte* buf, SQLLEN column_size, SQLLEN& buf_size);
        
        bool outbound_bool_column(SQLUSMALLINT column_number, SQLCHAR&);

        bool outbound_int32_column(SQLUSMALLINT column_number, SQLINTEGER&);

        bool outbound_int64_column(SQLUSMALLINT column_number, SQLBIGINT&);

        bool outbound_uint32_column(SQLUSMALLINT column_number, SQLUINTEGER&);

        bool outbound_uint64_column(SQLUSMALLINT column_number, SQLUBIGINT&);

        bool prepare(std::string_view query);

        bool execute();

        bool execute_direct(const char* query);

        bool fetch();

        bool more_results();

        bool close_cursor();

        bool is_valid() const
        {
            return statement_handle != SQL_NULL_HSTMT;
        }

    private:
        SQLHSTMT statement_handle = SQL_NULL_HSTMT;
    };
}