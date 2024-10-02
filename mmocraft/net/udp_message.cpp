#include "pch.h"
#include "udp_message.h"

#include "logging/logger.h"

namespace net
{
    void MessageRequest::set_request_address(const net::IPAddress& addr)
    {
        req_address.recipient_addr.sin_family = AF_INET;
        req_address.recipient_addr.sin_port = ::htons(addr.port);
        ::inet_pton(AF_INET, addr.ip.data(), &req_address.recipient_addr.sin_addr);
        
        req_address.recipient_addr_size = sizeof(req_address.recipient_addr);
    }

    bool MessageRequest::read_message()
    {
        auto transferred_bytes = ::recvfrom(
            _requester,
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

    bool MessageRequest::flush_send(bool is_reply) const
    {
        auto& addr = is_reply ? reply_address : req_address;

        auto transferred_bytes = ::sendto(
            _requester,
            cbegin(), int(size()),
            0,
            reinterpret_cast<const SOCKADDR*>(&addr.recipient_addr),
            addr.recipient_addr_size
        );

        if (transferred_bytes == SOCKET_ERROR) {
            LOG(error) << "sendto() failed with " << ::WSAGetLastError();
            return false;
        }

        LOG_IF(error, transferred_bytes != SOCKET_ERROR && transferred_bytes < size())
            << "sendto() successed partially";

        return true;
    }
}