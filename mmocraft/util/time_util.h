#pragma once

#include <chrono>
#include <ctime>

namespace util
{
	enum Second
	{
		
	};

	inline std::time_t current_timestmap()
	{
		return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	}
}