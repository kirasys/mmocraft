#include "pch.h"
#include "udp_message.h"

#include "logging/logger.h"

namespace net
{
    bool MessageRequest::read_message(win::Socket sock)
    {
        reply_address.sender = sock;

        auto transferred_bytes = ::recvfrom(
            sock,
            begin(), int(capacity()),
            0,
            reinterpret_cast<SOCKADDR*>(&reply_address.recipient_addr),
            &reply_address.recipient_addr_size
        );

        if (transferred_bytes == SOCKET_ERROR || transferred_bytes == 0) {
            auto errorcode = ::WSAGetLastError();
            CONSOLE_LOG_IF(error, errorcode != 10004 && errorcode != 10038)
                << "recvfrom() failed with :" << errorcode;
            return false;
        }

        set_size(transferred_bytes);
        return true;
    }

    void MessageRequest::send_reply(const MessageRequest& response)
    {
        auto transferred_bytes = ::sendto(
            reply_address.sender,
            response.cbegin(), int(response.size()),
            0,
            reinterpret_cast<SOCKADDR*>(&reply_address.recipient_addr),
            reply_address.recipient_addr_size
        );

        LOG_IF(error, transferred_bytes == SOCKET_ERROR)
            << "sendto() failed with " << ::WSAGetLastError();
        LOG_IF(error, transferred_bytes != SOCKET_ERROR && transferred_bytes < response.size())
            << "sendto() successed partially";
    }
}