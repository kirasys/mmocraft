#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>

#include "logging/error.h"
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
            ::strcpy_s(username, sizeof(username), src_packet.username.data);
            ::strcpy_s(password, sizeof(password), src_packet.password.data);
        }

        DeferredPacket<PacketHandshake>* next = nullptr;
        ConnectionLevelDescriptor connection_descriptor;
        char username[net::PacketFieldConstraint::max_username_length + 1];
        char password[net::PacketFieldConstraint::max_password_length + 1];
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

        template <typename PacketType>
        bool is_empty()
        {
            return get_head<PacketType>().load(std::memory_order_relaxed) == nullptr;
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

    // forward declaration.
    template<typename PacketType = PacketHandshake>
    struct DeferredPacketEvent;

    class DeferredPacketHandler
    {
    public:
        virtual void handle_deferred_packet(DeferredPacketEvent<PacketHandshake>*) = 0;
    };

    struct IDeferredPacketEvent
    {
        enum Status
        {
            Unused,
            Processing,
            Failed,
        };

        Status _status = Unused;

        Status& status()
        {
            return _status;
        }

        virtual void invoke_handler(DeferredPacketHandler&) = 0;

        template <typename PacketType>
        void delete_packets(DeferredPacket<PacketType>* head)
        {
            for (auto packet = head, next_packet = head; packet; packet = next_packet) {
                next_packet = packet->next;
                delete packet;
            }
        }
    };

    struct DeferredPacketResult
    {
        ConnectionLevelDescriptor connection_descriptor;
        error::ErrorCode result;
        DeferredPacketResult* next;
    };

    class DeferredPacketResultStack
    {
    public:
        void push(ConnectionLevelDescriptor desc, error::ErrorCode result);

        inline DeferredPacketResult* pop()
        {
            return head.exchange(nullptr);
        }

    private:
        std::atomic<DeferredPacketResult*> head{ nullptr };
    };

    template <typename PacketType>
    struct DeferredPacketEvent : IDeferredPacketEvent
    {
        DeferredPacket<PacketType>* head = nullptr;
        DeferredPacketResultStack result;

        void invoke_handler(DeferredPacketHandler& event_handler)
        {
            auto old_head = head;       // Note: save head first to avoid memory ordering issues.
            event_handler.handle_deferred_packet(this);

            _status = Status::Unused;    // Note: must use old_head because after staus changes, head also can be overwrited. 
            delete_packets(old_head);
        }
    };
}