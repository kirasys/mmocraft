#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <memory>

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
        DeferredPacket<PacketHandshake>(DescriptorType::Connection desc, const PacketHandshake& src_packet)
            : connection_descriptor{ DescriptorType::DeferredPacket(desc) }
        {
            ::strcpy_s(username, src_packet.username.data);
            ::strcpy_s(password, src_packet.password.data);
        }

        DeferredPacket<PacketHandshake>* next = nullptr;
        DescriptorType::DeferredPacket connection_descriptor;
        char username[net::PacketFieldConstraint::max_username_length + 1];
        char password[net::PacketFieldConstraint::max_password_length + 1];
    };

    // forward declaration.
    template<typename PacketType = PacketHandshake>
    class DeferredPacketEvent;

    struct DeferredPacketResult
    {
        DescriptorType::DeferredPacket connection_descriptor;
        error::ErrorCode error_code;
        DeferredPacketResult* next;
    };

    class DeferredPacketHandler
    {
    public:
        virtual void handle_deferred_packet(DeferredPacketEvent<PacketHandshake>*, const DeferredPacket<PacketHandshake>*) = 0;
    };

    class PacketEvent
    {
    public:
        enum State
        {
            Unused,
            Processing,
            Failed,
        };

        State _state = Unused;

        bool transit_state(State old_state, State new_state)
        {
            if (_state == old_state) {
                _state = new_state;
                return true;
            }
            return false;
        }

        virtual void invoke_handler(DeferredPacketHandler&) = 0;

        virtual bool is_exist_pending_packet() const = 0;

        virtual auto pop_pending_result() -> std::unique_ptr<DeferredPacketResult, void(*)(DeferredPacketResult*)> = 0;
    };


    template <typename PacketType>
    class DeferredPacketEvent: public PacketEvent
    {
    public:
        virtual void invoke_handler(DeferredPacketHandler& event_handler) override
        {
            auto head_ptr = pop_pending_packet();
            event_handler.handle_deferred_packet(this, head_ptr.get());

            transit_state(State::Processing, State::Unused);
        }

        virtual bool is_exist_pending_packet() const override
        {
            return pending_packet_head.load(std::memory_order_relaxed) != nullptr;
        }

        void push_packet(DescriptorType::Connection desc, const PacketType& src_packet)
        {
            auto new_packet = new DeferredPacket<PacketType>(desc, src_packet);

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

        void push_result(DescriptorType::DeferredPacket desc, error::ErrorCode error_code)
        {
            auto new_packet = new DeferredPacketResult{
                .connection_descriptor = desc,
                .error_code = error_code,
                .next = pending_result_head.load(std::memory_order_relaxed)
            };

            while (!pending_result_head.compare_exchange_weak(new_packet->next, new_packet,
                std::memory_order_release, std::memory_order_relaxed));
        }

        static void delete_results(DeferredPacketResult* head)
        {
            for (auto result = head, next_result = head; result; result = next_result) {
                next_result = result->next;
                delete result;
            }
        }

        using result_ptr_type = std::unique_ptr<DeferredPacketResult, decltype(delete_results)*>;

        auto pop_pending_result() -> result_ptr_type override
        {
            return result_ptr_type(
                pending_result_head.exchange(nullptr, std::memory_order_relaxed),
                delete_results
            );
        }

    private:
        std::atomic<DeferredPacket<PacketType>*> pending_packet_head{ nullptr };
        std::atomic<DeferredPacketResult*> pending_result_head{ nullptr };
    };
}