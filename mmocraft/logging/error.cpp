#include "pch.h"
#include "error.h"

#include <map>
#include <filesystem>
#include <cstdlib>

namespace error
{
	std::string_view to_string(ErrorCode::Network code)
	{
		static std::map< ErrorCode::Network, const char*> error_code_map = {
			{ ErrorCode::Network::SUCCESS, "SUCCESS"},
			{ ErrorCode::Network::CREATE_SOCKET_ERROR, "CREATE_SOCKET_ERROR"},
			{ ErrorCode::Network::BIND_ERROR, "BIND_ERROR"},
			{ ErrorCode::Network::LISTEN_ERROR, "LISTEN_ERROR"},
			{ ErrorCode::Network::ACCEPTEX_LOAD_ERROR, "ACCEPTEX_LOAD_ERROR"},
			{ ErrorCode::Network::ACCEPTEX_FAIL_ERROR, "ACCEPTEX_FAIL_ERROR"},
		};
		return error_code_map[code];
	}

	NetworkException::NetworkException(ErrorCode::Network code,
		int os_error_code,
		std::string_view summary,
		const std::source_location& location)
	{
		m_message << std::filesystem::path(location.file_name()).filename() << '('
			<< location.line() << ':'
			<< location.column() << ") :";

		m_message << "Network Exception(" << to_string(code) << ") occcured";
		if (os_error_code)
			m_message << "\nos error code:" << os_error_code;
		if (summary.size())
			m_message << "\nsummary:" << summary;
	}
}