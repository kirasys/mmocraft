#pragma once
#include <string_view>

#include <sql.h>
#include <sqlext.h>

#include "util/common_util.h"
#include "logging/logger.h"
#include "win/win_type.h"

namespace database
{
	class SQLStatement : util::NonCopyable
	{
	public:
		SQLStatement(SQLHDBC);

		~SQLStatement();

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

		bool inbound_integer_parameter(SQLUSMALLINT parameter_number, SQLINTEGER&);

		bool inbound_chars_parameter(SQLUSMALLINT parameter_number, const char*, SQLLEN, SQLLEN&);

		bool inbound_null_terminated_string_parameter(SQLUSMALLINT parameter_number, const char*, SQLLEN);

		bool outbound_integer_column(SQLUSMALLINT column_number, SQLINTEGER&);

		bool outbound_unsigned_integer_column(SQLUSMALLINT column_number, SQLUINTEGER&);

		bool prepare(std::string_view query);

		bool execute();

		bool fetch();

		bool close_cursor();

		bool is_valid() const
		{
			return statement_handle != SQL_NULL_HSTMT;
		}

	private:
		SQLHSTMT statement_handle = SQL_NULL_HSTMT;
	};

	constexpr const char* sql_player_auth = "SELECT COUNT(*) FROM player WHERE username = ? AND password = dbo.GetPasswordHash(?)";

	class PlayerAuthSQL : public SQLStatement
	{
	public:
		PlayerAuthSQL(SQLHDBC);

		~PlayerAuthSQL() = default;

		bool authenticate(const char* username, const char* password);

	private:
		char _username[16 + 1];
		char _password[32 + 1];

		SQLUINTEGER selected_player_count = 0;
	};
}