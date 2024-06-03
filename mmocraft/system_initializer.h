#pragma once

#include <exception>

namespace setup
{
	void initialize_system();

	void add_termination_handler(std::terminate_handler handler);
}