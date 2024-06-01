#include "pch.h"
#include "deferred_packet.h"

namespace net
{
    void DeferredPacketResultStack::push(DescriptorType::DeferredPacket desc, error::ErrorCode error_code)
    {
        auto new_packet = new DeferredPacketResult{
            .connection_descriptor = desc,
            .error_code = error_code,
            .next = head.load(std::memory_order_relaxed)
        };

        while (!head.compare_exchange_weak(new_packet->next, new_packet,
            std::memory_order_release, std::memory_order_relaxed));
    }
}