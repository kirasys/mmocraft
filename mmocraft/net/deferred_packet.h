#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>

#include "io/task.h"
#include "logging/error.h"
#include "net/packet.h"
#include "net/connection_key.h"
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
        DeferredPacket<PacketHandshake>(net::ConnectionKey key, const PacketHandshake& src_packet)
            : connection_key{ key }
        {
            ::strcpy_s(username, src_packet.username.data);
            ::strcpy_s(password, src_packet.password.data);
        }

        DeferredPacket<PacketHandshake>* next = nullptr;
        net::ConnectionKey connection_key;
        char username[net::PacketFieldConstraint::max_username_length + 1];
        char password[net::PacketFieldConstraint::max_password_length + 1];
    };

    template <typename PacketType, typename HandlerClass>
    class DeferredPacketTask: public io::Task
    {
    public:
        using handler_type = void (HandlerClass::*const)(io::Task*, const DeferredPacket<PacketType>*);

        DeferredPacketTask(handler_type handler, HandlerClass* handler_inst = nullptr)
            : _handler{ handler }
            , _handler_inst{ handler_inst }
        { }

        virtual void invoke_handler(ULONG_PTR task_handler_inst) override
        {
            auto head_ptr = pop_pending_packet();

            std::invoke(_handler,
                _handler_inst ? *_handler_inst : *reinterpret_cast<HandlerClass*>(task_handler_inst),
                this,
                head_ptr.get()
            );

            transit_state(State::Processing, State::Unused);
        }

        virtual bool exists() const override
        {
            return pending_packet_head.load(std::memory_order_relaxed) != nullptr;
        }

        void push_packet(net::ConnectionKey key, const PacketType& src_packet)
        {
            auto new_packet = new DeferredPacket<PacketType>(key, src_packet);

            new_packet->next = pending_packet_head.load(std::memory_order_relaxed);

            while (!pending_packet_head.compare_exchange_weak(new_packet->next, new_packet,
                std::memory_order_release, std::memory_order_relaxed));
        }

        static void delete_packets(DeferredPacket<PacketType>* head)
        {
            for (auto packet = head, next_packet = head; packet; packet = next_packet) {
                next_packet = packet->next;
                delete packet;
            }
        }

        auto pop_pending_packet()
        {
            return std::unique_ptr<DeferredPacket<PacketType>, decltype(delete_packets)*>(
                pending_packet_head.exchange(nullptr, std::memory_order_relaxed),
                delete_packets
            );
        }

    private:
        handler_type _handler;
        HandlerClass* _handler_inst = nullptr;

        std::atomic<DeferredPacket<PacketType>*> pending_packet_head{ nullptr };
    };
}