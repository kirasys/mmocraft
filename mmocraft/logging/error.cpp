#include "error.h"

#include <map>
#include <filesystem>
#include <cstdlib>

namespace logging
{
	const std::map<ErrorMessage, const char*> error_msg_map = {
		{ErrorMessage::FILE_OPEN_ERROR, "Fail to open file: "},
		{ErrorMessage::INVALID_SOCKET_ERROR, "Invalid socket"},
	};

	ErrorStream::ErrorStream(const std::source_location& location, bool exit_after_print)
		: m_exit_after_print{ exit_after_print }
	{
		m_buf << std::filesystem::path(location.file_name()).filename() << '('
			<< location.line() << ':'
			<< location.column() << ") `"
			<< location.function_name() << "`: ";
	}

	ErrorStream& ErrorStream::operator<<(ErrorMessage type)
	{
		if (error_msg_map.find(type) == error_msg_map.end())
			m_buf << "Unknown error type (" << type << ')';
		else
			m_buf << error_msg_map.at(type);

		return *this;
	}

	ErrorStream::~ErrorStream()
	{
		// TODO: does it thread-safe?
		std::cerr << m_buf.str() << std::endl;

		if (m_exit_after_print)
			std::exit(0);
	}

	ErrorStream cerr(const std::source_location& location) {
		return ErrorStream{ location, false };
	}

	ErrorStream cfatal(const std::source_location& location) {
		return ErrorStream{ location, true };
	}


	/*
	std::ostream& operator<<(std::ostream& os, ErrorCode::Socket code)
	{
		static std::map< ErrorCode::Socket, const char*> error_code_map = {
			{ ErrorCode::Socket::SUCCESS, "SUCCESS"},
			{ ErrorCode::Socket::CREATE_SOCKET_ERROR, "CREATE_SOCKET_ERROR"},
			{ ErrorCode::Socket::BIND_ERROR, "BIND_ERROR"},
			{ ErrorCode::Socket::LISTEN_ERROR, "LISTEN_ERROR"},
			{ ErrorCode::Socket::ACCEPTEX_LOAD_ERROR, "ACCEPTEX_LOAD_ERROR"},
			{ ErrorCode::Socket::ACCEPTEX_FAIL_ERROR, "ACCEPTEX_FAIL_ERROR"},
		};

		if (error_code_map.find(code) == error_code_map.end())
			return os << "UNKNOWN ERROR CODE(" << int(code) << ")";

		return os << error_code_map.at(code);
	}
	*/

}

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
		std::string_view summary,
		const std::source_location& location)
	{
		m_message << std::filesystem::path(location.file_name()).filename() << '('
			<< location.line() << ':'
			<< location.column() << ") :";

		m_message << "Network Exception(" << to_string(code) << ") occcured";
		if (summary.size())
			m_message << "- " << summary;
	}
}