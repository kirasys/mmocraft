#pragma once

#include "net/message_id.h"

namespace net
{

    constexpr std::size_t REQUEST_MESSAGE_SIZE = 2048;
    constexpr std::size_t RESPONSE_MESSAGE_SIZE = 2048;

    class MessageRequest
    {
    public:
        MessageRequest(::net::MessageID msg_id = ::net::MessageID::Invalid_MessageID)
        {
            reset(msg_id);
        }

        consteval static std::size_t size_of_header()
        {
            return 1;
        }

        ::net::MessageID message_id() const
        {
            return ::net::MessageID(_buf[0]);
        }

        void set_message_id(::net::MessageID msg_id)
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

        void reset(::net::MessageID msg_id = ::net::MessageID::Invalid_MessageID)
        {
            _size = size_of_header();
            _buf[0] = msg_id;
        }

        char* begin()
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
        void set_message(MessageType msg)
        {
            msg.SerializeToArray(begin_message(), int(message_capacity()));
            set_message_size(msg.ByteSizeLong());
        }

    private:
        std::size_t _size;
        char _buf[REQUEST_MESSAGE_SIZE];
    };

    using MessageResponse = MessageRequest;

    class MessageHandler
    {
    public:
        virtual bool handle_message(const MessageRequest&, MessageResponse& response) = 0;
    };
}