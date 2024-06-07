#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>

#include "logging/error.h"
#include "net/packet.h"
#include "net/connection.h"
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
        DeferredPacket<PacketHandshake>(Connection::Descriptor* desc, const PacketHandshake& src_packet)
            : connection_descriptor{ desc }
        {
            ::strcpy_s(username, src_packet.username.data);
            ::strcpy_s(password, src_packet.password.data);
        }

        DeferredPacket<PacketHandshake>* next = nullptr;
        Connection::Descriptor* connection_descriptor;
        char username[net::PacketFieldConstraint::max_username_length + 1];
        char password[net::PacketFieldConstraint::max_password_length + 1];
    };

    struct DeferredPacketResult
    {
        Connection::Descriptor* connection_descriptor;
        error::ResultCode result_code;
        DeferredPacketResult* next;
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

        virtual void invoke_handler(ULONG_PTR event_handler_inst) = 0;

        virtual void invoke_result_handler(void* result_handler_inst) = 0;

        virtual bool is_exist_pending_packet() const = 0;

        virtual bool is_exist_pending_result() const = 0;

        virtual void push_result(Connection::Descriptor*, error::ErrorCode) = 0;
    };


    template <typename PacketType, typename HandlerClass>
    class DeferredPacketEvent: public PacketEvent
    {
    public:
        using handler_type = void (HandlerClass::*const)(PacketEvent*, const DeferredPacket<PacketType>*);
        using result_handler_type = void (HandlerClass::* const)(const DeferredPacketResult*);

        DeferredPacketEvent(handler_type a_handler, result_handler_type a_result_handler)
            : _handler{ a_handler }
            , _result_handler{ a_result_handler }
        { }

        virtual void invoke_handler(ULONG_PTR event_handler_inst) override
        {
            auto head_ptr = pop_pending_packet();

            std::invoke(_handler,
                *reinterpret_cast<HandlerClass*>(event_handler_inst),
                this,
                head_ptr.get()
            );

            transit_state(State::Processing, State::Unused);
        }

        virtual void invoke_result_handler(void* result_handler_inst) override
        {
            auto result_ptr = pop_pending_result();

            std::invoke(_result_handler,
                *reinterpret_cast<HandlerClass*>(result_handler_inst),
                result_ptr.get()
            );
        }

        virtual bool is_exist_pending_packet() const override
        {
            return pending_packet_head.load(std::memory_order_relaxed) != nullptr;
        }

        virtual bool is_exist_pending_result() const override
        {
            return pending_result_head.load(std::memory_order_relaxed) != nullptr;
        }

        void push_packet(Connection::Descriptor* desc, const PacketType& src_packet)
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

        void push_result(Connection::Descriptor* desc, error::ErrorCode error_code) override
        {
            auto new_packet = new DeferredPacketResult{
                .connection_descriptor = desc,
                .result_code = error_code,
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

        auto pop_pending_result() -> result_ptr_type
        {
            return result_ptr_type(
                pending_result_head.exchange(nullptr, std::memory_order_relaxed),
                delete_results
            );
        }

    private:
        handler_type _handler;
        result_handler_type _result_handler;

        std::atomic<DeferredPacket<PacketType>*> pending_packet_head{ nullptr };
        std::atomic<DeferredPacketResult*> pending_result_head{ nullptr };
    };
}