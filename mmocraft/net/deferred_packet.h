#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>

#include "net/packet.h"
#include "net/connection_descriptor.h"
#include "util/common_util.h"

namespace net
{
    template <typename PacketType>
    struct DeferredPacket
    {
        
    };

    template <>
    struct DeferredPacket<PacketHandshake>
    {
        DeferredPacket<PacketHandshake>(ConnectionLevelDescriptor desc, const PacketHandshake& src_packet)
            : connection_descriptor{ desc }
        {
            ::strcpy_s(_username, sizeof(_username), src_packet.username.data);
            ::strcpy_s(_password, sizeof(_password), src_packet.password.data);
        }

        DeferredPacket<PacketHandshake>* next = nullptr;
        ConnectionLevelDescriptor connection_descriptor;
        char _username[net::PacketFieldConstraint::max_username_length + 1];
        char _password[net::PacketFieldConstraint::max_password_length + 1];
    };

    class DeferredPacketStack : util::NonCopyable, util::NonMovable
    {
    public:
        DeferredPacketStack() = default;

        template <typename PacketType>
        void push(ConnectionLevelDescriptor desc, const PacketType& src_packet)
        {
            auto& head = get_head<PacketType>();
            auto new_packet = new DeferredPacket<PacketType>(desc, src_packet);

            new_packet->next = head.load(std::memory_order_relaxed);

            while (!head.compare_exchange_weak(new_packet->next, new_packet,
                std::memory_order_release, std::memory_order_relaxed));
        }

        template <typename PacketType>
        DeferredPacket<PacketType>* pop()
        {
            return get_head<PacketType>().exchange(nullptr);
        }

    private:
        template <typename PacketType>
        std::atomic<DeferredPacket<PacketType>*>& get_head()
        {
            assert(false);
            return nullptr;
        }

        template <>
        std::atomic<DeferredPacket<PacketHandshake>*>& get_head<PacketHandshake>()
        {
            return head_packet_handshake;
        }

        std::atomic<DeferredPacket<PacketHandshake>*> head_packet_handshake{ nullptr };
    };
}