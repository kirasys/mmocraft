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
            , cpe_support{ src_packet.cpe_magic == 0x42 }
        {
            ::memcpy_s(username, sizeof(username), src_packet.username.data(), src_packet.username.size());
            ::memcpy_s(password, sizeof(password), src_packet.password.data(), src_packet.password.size());
        }

        DeferredPacket<PacketHandshake>* next = nullptr;
        net::ConnectionKey connection_key;
        char username[net::PacketFieldConstraint::max_username_length + 1] = { 0 };
        char password[net::PacketFieldConstraint::max_password_length + 1] = { 0 };
        bool cpe_support;
    };

    template <>
    struct DeferredPacket<PacketChatMessage>
    {
        DeferredPacket<PacketChatMessage>(net::ConnectionKey key, const PacketChatMessage& src_packet)
            : connection_key{ key }
            , player_id{ src_packet.player_id }
            , message_length{ src_packet.message.size()}
        {
            ::memcpy_s(message, sizeof(message), src_packet.message.data(), message_length);
        }

        static std::size_t serialize(std::vector<const DeferredPacket<PacketChatMessage>*>& packets, std::unique_ptr<std::byte[]>& serialized_data)
        {
            auto data_size = packets.size() * net::PacketChatMessage::packet_size;
            serialized_data.reset(new std::byte[data_size]);

            auto buf_start = serialized_data.get();

            for (auto packet : packets) {
                PacketStructure::write_byte(buf_start, net::PacketChatMessage::packet_id);
                PacketStructure::write_byte(buf_start, packet->player_id);
                PacketStructure::write_string(buf_start, packet->message, packet->message_length);
            }

            return data_size;
        }

        DeferredPacket<PacketChatMessage>* next = nullptr;
        net::ConnectionKey connection_key;
        
        game::PlayerID player_id;
        std::byte message[net::PacketFieldConstraint::max_string_length] = {};
        std::size_t message_length;
    };

    template <typename PacketType, typename HandlerClass>
    class DeferredPacketTask: public io::Task
    {
    public:
        using handler_type = void (HandlerClass::*const)(io::Task*, const DeferredPacket<PacketType>*);

        DeferredPacketTask(handler_type handler, HandlerClass* handler_inst, std::size_t interval_ms = 0)
            : io::Task{ interval_ms }
            , _handler{ handler }
            , _handler_inst{ handler_inst }
        { }

        virtual void invoke_handler(ULONG_PTR task_handler_inst) override
        {
            auto head_ptr = pop_pending_packet();
            
            try {
                std::invoke(_handler,
                    _handler_inst ? *_handler_inst : *reinterpret_cast<HandlerClass*>(task_handler_inst),
                    this,
                    head_ptr.get()
                );
            }
            catch (...) {
                CONSOLE_LOG(error) << "Unexpected error occured at deferred packet handler";
            }

            set_state(io::Task::Unused);
        }

        virtual bool ready() const override
        {
            return io::Task::ready() && pending_packet_head.load(std::memory_order_relaxed) != nullptr;
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