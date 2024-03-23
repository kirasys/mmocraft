#include <iostream>
#include <winsock2.h>

#include "config/config.h"
#include "util/deferred_call.h"
#include "net/server_core.h"
#include "logging/error.h"

int main()
{
	WSADATA wsaData;
	if (int result = ::WSAStartup(MAKEWORD(2, 2), &wsaData); result != 0) {
		std::cerr << "WSAStartup() failed: " << result << std::endl;
		return 0;
	}

	util::defer deinit = [] {
		::WSACleanup();
	};

	try {
		auto server_core = net::ServerCore("121.0.0.1", 12345);
		server_core.serve_forever();
	}
	catch (const error::NetworkException &code) {
		std::cout << code.what() << std::endl;
	}
}