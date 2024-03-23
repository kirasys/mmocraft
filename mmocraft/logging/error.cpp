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
}