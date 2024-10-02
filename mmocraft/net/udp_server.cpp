#include "pch.h"
#include "udp_server.h"

#include "net/server_communicator.h"
#include "config/config.h"
#include "config/constants.h"
#include "logging/logger.h"

namespace net
{
    UdpServer::UdpServer(net::MessageHandler& server_inst)
        : listen_sock{ net::socket_protocol_id::udp_v4 }
        , app_server{ server_inst }
        , _communicator{ listen_sock }
    {
        if (not listen_sock.set_socket_option(SO_RCVBUF, config::memory::udp_kernel_recv_buffer_size) ||
            not listen_sock.set_socket_option(SO_SNDBUF, config::memory::udp_kernel_send_buffer_size)) {
            CONSOLE_LOG(fatal) << "Fail to set socket option";
        }
    }

    void UdpServer::reset()
    {
        listen_sock.reset(net::socket_protocol_id::udp_v4);
        is_terminated = true;

        for (auto& event_thread : event_threads)
            event_thread.join();

        event_threads.clear();
    }

    void UdpServer::start_network_io_service(std::string_view ip, int port, std::size_t num_of_event_threads)
    {
        if (not listen_sock.bind(ip, port)) {
            CONSOLE_LOG(fatal) << "Couldn't bind server " << ip << ':' << port;
            return;
        }

        CONSOLE_LOG(info) << "Listening to " << ip << ':' << port << "...";

        for (unsigned i = 0; i < num_of_event_threads; i++)
            event_threads.emplace_back(spawn_event_loop_thread());

    }

    std::thread UdpServer::spawn_event_loop_thread()
    {
        return std::thread([](UdpServer* udp_server) {
            udp_server->run_event_loop_forever(0);
            }, this);
    }

    void UdpServer::run_event_loop_forever(DWORD)
    {
        net::MessageRequest request;
        request.set_requester(listen_sock.get_handle());

        while (not is_terminated) {
            if (not request.read_message())
                continue;

            // handle message and send reply.
            handle_message(request);
        }
    }

    bool UdpServer::handle_message(net::MessageRequest& request)
    {
        // invoke common message handlers first.

        auto common_handler_success = _communicator.handle_common_message(request);

        if (not common_handler_success.value_or(true)) {
            CONSOLE_LOG(error) << "Common handler failed" << request.message_id();
            return false;
        }

        // invoke user-defined handlers if exists

        if (app_server.handle_message(request) || common_handler_success.value_or(false))
            return true;

        CONSOLE_LOG(error) << "Fail to handle message: " << request.message_id();
        return false;
    }
}