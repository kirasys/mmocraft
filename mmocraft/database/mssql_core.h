#pragma once

#include "win/win_type.h"

#include <sql.h>
#include <sqlext.h>
#include <string_view>

#include "proto/generated/config.pb.h"
#include "util/common_util.h"
#include "logging/logger.h"

namespace database
{
	class DatabaseCore : util::NonCopyable, util::NonMovable
	{
	public:
		enum class State
		{
			uninitialized,
			initialized,
			connected,
			disconnected,
		};

		State status() const
		{
			return _state;
		}

		DatabaseCore();

		~DatabaseCore();

		static bool connect_server(std::string_view connection_string);

		static bool connect_server_with_login(const config::Configuration_Database&);

		void disconnect();

		static auto get_connection()
		{
			return connection_handle;
		}

		static void logging_current_connection_error(RETCODE);

	private:
		State _state = State::uninitialized;

		SQLHENV environment_handle;
		static SQLHDBC connection_handle;
	};
}