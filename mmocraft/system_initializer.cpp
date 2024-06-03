#include "pch.h"
#include "system_initializer.h"

#include <cstdlib>
#include <csignal>
#include <vector>

#include "logging/logger.h"
#include "net/connection_descriptor.h"
#include "net/socket.h"

namespace
{
	std::vector<std::terminate_handler> system_terminatio_handlers;

	void termination_routine()
	{
		for (auto handler : system_terminatio_handlers)
			handler();

		std::exit(0);
	}

	void termination_routine_for_signal(int signal)
	{
		return;
	}
}

namespace setup
{
	void initialize_system()
	{
		std::set_terminate(termination_routine);

		std::signal(SIGTERM, termination_routine_for_signal);
		std::signal(SIGSEGV, termination_routine_for_signal);
		std::signal(SIGINT, termination_routine_for_signal);
		std::signal(SIGABRT, termination_routine_for_signal);

		// log system
		logging::initialize_system();

		// network system
		net::Socket::initialize_system();
		net::ConnectionDescriptor::initialize_system();
	}

	void add_termination_handler(std::terminate_handler handler)
	{
		system_terminatio_handlers.push_back(handler);
	}
}