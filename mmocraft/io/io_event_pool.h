#pragma once

#include <type_traits>
#include <cstdlib>

#include "io_event.h"
#include "win/object_pool.h"

namespace io
{
	class IoEventPool
	{
	public:
		virtual IoAcceptEvent* new_accept_event() = 0;
		virtual void delete_event(IoAcceptEvent*) = 0;

		virtual IoSendEvent* new_send_event() = 0;
		virtual void delete_event(IoSendEvent*) = 0;

		virtual IoRecvEvent* new_recv_event() = 0;
		virtual void delete_event(IoRecvEvent*) = 0;
	};

	/// 오브젝트 풀을 통해 IoContext 구조체를 할당하는 클래스.
	/// 각각의 풀은 VirtualAlloc으로 연속된 공간에 할당된다.
	class IoEventObjectPool : public IoEventPool
	{	
	public:
		IoEventObjectPool(std::size_t max_capacity)
			: accept_event_pool{ 1 }
			, send_event_pool{ max_capacity }
			, send_event_data_pool { max_capacity }
			, recv_event_pool{ max_capacity }
			, recv_event_data_pool{ max_capacity + 1}
		{ }

		IoAcceptEvent* new_accept_event()
		{
			auto io_buf = recv_event_data_pool.new_object_raw();
			return accept_event_pool.new_object_raw(io_buf);
		}

		void delete_event(IoAcceptEvent* event)
		{
			recv_event_data_pool.free_object(static_cast<IoRecvEventData*>(&event->data));
			accept_event_pool.free_object(event);
		}

		IoSendEvent* new_send_event()
		{
			auto io_buf = send_event_data_pool.new_object_raw();
			return send_event_pool.new_object_raw(io_buf);
		}

		void delete_event(IoSendEvent* event)
		{
			send_event_data_pool.free_object(static_cast<IoSendEventData*>(&event->data));
			send_event_pool.free_object(event);
		}

		IoRecvEvent* new_recv_event()
		{
			auto io_buf = recv_event_data_pool.new_object_raw();
			return recv_event_pool.new_object_raw(io_buf);
		}

		void delete_event(IoRecvEvent* event)
		{
			recv_event_data_pool.free_object(static_cast<IoRecvEventData*>(&event->data));
			recv_event_pool.free_object(event);
		}

	private:
		win::ObjectPool<IoAcceptEvent> accept_event_pool;

		win::ObjectPool<IoSendEvent> send_event_pool;
		win::ObjectPool<IoSendEventData> send_event_data_pool;

		win::ObjectPool<IoRecvEvent> recv_event_pool;
		win::ObjectPool<IoRecvEventData> recv_event_data_pool;
	};
}