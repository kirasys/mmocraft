#include "error.h"

#include <map>
#include <filesystem>

namespace logging
{
	const std::map<ErrorMessage, const char*> error_msg_map = {
		{ErrorMessage::FILE_OPEN_ERROR, "Fail to open file: "},
		{ErrorMessage::INVALID_SOCKET_ERROR, "Invalid socket"},
	};
	
	ErrorStream::ErrorStream(const std::source_location& location)
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
	}

	ErrorStream cerr(const std::source_location& location) {
		return ErrorStream{ location };
	}
}