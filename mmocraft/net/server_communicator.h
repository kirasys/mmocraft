#pragma once

#include <initializer_list>
#include <shared_mutex>

#include "net/udp_server.h"
#include "net/packet_id.h"
#include "proto/generated/protocol.pb.h"

namespace net
{
    constexpr int UDP_MESSAGE_RETRANSMISSION_PERIOD = 3000;

    struct ServerInfo
    {
        std::string ip = "";
        int port = 0;
    };

    class ServerCommunicator
    {
    public:
        ServerCommunicator(net::UdpServer& src)
            : _source{ src }
        { }

        net::ServerInfo get_server(protocol::ServerType);

        void register_server(protocol::ServerType, const net::ServerInfo&);

        bool announce_server(protocol::ServerType, const net::ServerInfo&);

        bool fetch_server_async(protocol::ServerType);

        bool fetch_server(protocol::ServerType);

        static bool read_message(net::Socket&, net::MessageRequest&, struct sockaddr_in& sender_addr, int& sender_addr_size);

        static bool read_message(net::Socket&, net::MessageRequest&);

        static bool send_message_reliably(const char* ip, int port, const net::MessageRequest&, net::MessageResponse&, int retry_count = std::numeric_limits<int>::max());

        static bool fetch_config(const char* router_ip, int router_port, protocol::ServerType target, net::MessageResponse&);

    private:
        net::UdpServer& _source;

        std::shared_mutex server_table_mutex;
        net::ServerInfo _servers[protocol::ServerType::SIZE];
    };

    class PacketRequest
    {
    public:
        PacketRequest(const net::MessageRequest& request)
        {
            _message.ParseFromArray(request.begin_message(), int(request.message_size()));
        }

        net::PacketID packet_id() const
        {
            return net::PacketID(_message.packet_data()[0]);
        }

        const std::byte* packet_data() const
        {
            return reinterpret_cast<const std::byte*>(_message.packet_data().data());
        }

    private:
        protocol::PacketHandleRequest _message;
    };

    class PacketResponse
    {
    public:
        PacketResponse(net::MessageResponse& response)
            : _response{ response }
        {

        }

        ~PacketResponse()
        {
            if (not _response_data.empty()) {
                protocol::PacketHandleResponse packet_handle_response;
                packet_handle_response.set_result_data(std::move(_response_data));
                _response.set_message(packet_handle_response);
            }
        }

        std::string& response_data()
        {
            return _response_data;
        }

    private:
        net::MessageResponse& _response;
        std::string _response_data;
    };
}