#include "config/config.h"
#include "util/deferred_call.h"
#include "net/socket.h"

#include <iostream>
#include <winsock2.h>

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

	//decltype(auto) conf = config::get_config();
	//std::cout << conf.loaded << std::endl;

	auto sock = net::Socket::create_socket(net::SocketType::TCPv4);
	std::cout << sock.is_valid() << std::endl;
	
	if (auto errorcode = sock.bind("122.0.0.1", 1234))
		std::cout << "fail to bind (" << errorcode << ')' << std::endl;

	if (auto errorcode = sock.listen())
		std::cout << "fail to listen(" << errorcode << ')' << std::endl;
}