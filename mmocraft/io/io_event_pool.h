#pragma once

#include <utility>
#include <type_traits>
#include <cstdlib>

#include "io_event.h"
#include "win/object_pool.h"

namespace io
{
    /// 오브젝트 풀을 통해 IoContext 구조체를 할당하는 클래스.
    /// 각각의 풀은 VirtualAlloc으로 연속된 공간에 할당된다.
    class IoEventObjectPool
    {	
    public:

        IoEventObjectPool(std::size_t max_capacity)
            : accept_event_pool{ 1 }
            , recv_event_pool{ max_capacity }
            , recv_event_data_pool{ max_capacity + 1}

            , send_event_pool{ 2 * max_capacity }
            , send_event_data_pool{ max_capacity }
            , send_event_lockfree_data_pool{ max_capacity }
        { }

        auto new_accept_event_data()
        {
            return recv_event_data_pool.new_object();
        }

        auto new_accept_event(IoAcceptEventData *io_data)
        {
            return accept_event_pool.new_object(io_data);
        }

        auto new_recv_event_data()
        {
            return recv_event_data_pool.new_object();
        }

        auto new_recv_event(IoRecvEventData* io_data)
        {
            return recv_event_pool.new_object(io_data);
        }

        auto new_send_event_data()
        {
            return send_event_data_pool.new_object();
        }

        auto new_send_event_lockfree_data()
        {
            return send_event_lockfree_data_pool.new_object();
        }

        auto new_send_event(IoEventData* io_data)
        {
            return send_event_pool.new_object(io_data);
        }

    private:
        win::ObjectGlobalPool<IoAcceptEvent> accept_event_pool;

        win::ObjectGlobalPool<IoRecvEvent> recv_event_pool;
        win::ObjectGlobalPool<IoRecvEventData> recv_event_data_pool;

        win::ObjectGlobalPool<IoSendEvent> send_event_pool;
        win::ObjectGlobalPool<IoSendEventData> send_event_data_pool;
        win::ObjectGlobalPool<IoSendEventLockFreeData> send_event_lockfree_data_pool;
    };

    using IoEventPool = IoEventObjectPool;
}