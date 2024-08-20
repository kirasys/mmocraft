#pragma once
#include <sql.h>
#include <sqlext.h>
#include <string_view>

#include "proto/generated/config.pb.h"
#include "util/common_util.h"
#include "logging/logger.h"
#include "win/win_type.h"

namespace database
{
	class DatabaseCore : util::NonCopyable, util::NonMovable
	{
	public:
		enum State
		{
			Uninitialized,
			Initialized,
			Connected,
			Disconnected,
		};

		State status() const
		{
			return _state;
		}

		DatabaseCore();

		~DatabaseCore();

		bool connect(std::string_view connection_string);

		bool connect_with_password(const config::Configuration_Database&);

		void disconnect();

		auto get_connection_handle() const
		{
			return connection_handle;
		}

		void logging_current_connection_error(RETCODE) const;

	private:

		State _state = Uninitialized;

		SQLHENV environment_handle = NULL;
		SQLHDBC connection_handle = NULL;
	};

	bool connect_database_server(database::DatabaseCore* db_core, const config::Configuration_Database& conf);
}