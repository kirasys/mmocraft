#pragma once

#include "net/packet_id.h"
#include "net/message_id.h"
#include "net/connection_key.h"
#include "win/win_type.h"
#include "proto/generated/protocol.pb.h"
#include "logging/logger.h"
#include "config/constants.h"

namespace net
{
    struct IPAddress
    {
        std::string ip = "";
        int port = 0;
    };

    class MessageRequest
    {
    public:
        MessageRequest(::net::message_id::value msg_id = ::net::message_id::invalid)
        {
            reset(msg_id);
        }

        MessageRequest(const MessageRequest&) = default;

        consteval static std::size_t size_of_header()
        {
            return 1;
        }

        ::net::message_id::value message_id() const
        {
            return ::net::message_id::value(_buf[0]);
        }

        void set_message_id(::net::message_id::value msg_id)
        {
            _buf[0] = char(msg_id);
        }

        void set_size(std::size_t data_size)
        {
            _size = std::min(data_size, capacity());
        }

        void set_message_size(std::size_t msg_size)
        {
            _size = std::min(msg_size + size_of_header(), capacity());
        }

        void set_request_address(const net::IPAddress&);

        void set_requester(win::Socket s)
        {
            _requester = s;
        }

        std::size_t size() const
        {
            return _size;
        }

        std::size_t message_size() const
        {
            return _size - size_of_header();
        }

        std::size_t capacity() const
        {
            return sizeof(_buf);
        }

        std::size_t message_capacity() const
        {
            return capacity() - size_of_header();
        }

        void reset(::net::message_id::value msg_id = ::net::message_id::invalid)
        {
            _size = size_of_header();
            _buf[0] = msg_id;
        }

        char* begin()
        {
            return _buf;
        }

        const char* cbegin() const
        {
            return _buf;
        }

        char* end()
        {
            return _buf + capacity();
        }

        char* begin_message()
        {
            return _buf + size_of_header();
        }

        const char* begin_message() const
        {
            return _buf + size_of_header();
        }

        char* end_message()
        {
            return _buf + size();
        }

        template <typename MessageType>
        void set_message(const MessageType& msg)
        {
            msg.SerializeToArray(begin_message(), int(message_capacity()));
            set_message_size(msg.ByteSizeLong());
        }

        template <typename MessageType>
        bool parse_message(MessageType& msg)
        {
            return msg.ParseFromArray(begin_message(), int(message_size()));
        }

        bool read_message();

        bool flush_send(bool is_reply = false) const;

        template <typename MessageType>
        bool send_message(const MessageType& msg)
        {
            set_message(msg);
            return flush_send();
        }

        template <typename MessageType>
        bool send_reply(const MessageType& msg)
        {
            set_message(msg);
            return flush_send(true);
        }

    private:
        std::size_t _size;
        char _buf[config::memory::udp_buffer_size];

        win::Socket _requester;

        struct SocketAddress {
            struct sockaddr_in recipient_addr;
            int recipient_addr_size = sizeof(recipient_addr);
        } req_address, reply_address;
    };

    using MessageResponse = MessageRequest;

    template <typename PacketType>
    class PacketMessage
    {
    public:

        PacketMessage(const net::MessageRequest& request)
            : _valid{ _message.ParseFromArray(request.begin_message(), int(request.message_size())) }
            , _packet{ packet_data() }
        {
            
        }

        bool is_valid() const
        {
            return _valid;
        }

        const auto& packet() const
        {
            return _packet;
        }

        const std::byte* packet_data() const
        {
            return reinterpret_cast<const std::byte*>(_message.packet_data().data());
        }

        auto connection_key() const
        {
            return net::ConnectionKey{ _message.connection_key() };
        }

    private:

        protocol::PacketHandleRequest _message;

        // Warning: _valid should be located after _message.
        bool _valid = false;

        PacketType _packet;
    };
}