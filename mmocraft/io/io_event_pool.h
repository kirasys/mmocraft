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
		template <typename EventType, typename EventDataType>
		class IoEventPtr
		{
		public:
			IoEventPtr(win::ObjectPool<EventType>::ScopedID&& io_event, win::ObjectPool<EventDataType>::ScopedID&& io_buf)
				: _io_event{ std::move(io_event) }, _io_buf{ std::move(io_buf) }
			{ }

			auto get()
			{
				return win::ObjectPool<EventType>::find_object(_io_event);
			}

			bool is_valid() const
			{
				return _io_event.is_valid() && _io_buf.is_valid();
			}

		private:
			win::ObjectPool<EventType>::ScopedID _io_event;
			win::ObjectPool<EventDataType>::ScopedID _io_buf;
		};

		using IoAcceptEventPtr = IoEventPtr<IoAcceptEvent, IoRecvEventData>;
		using IoRecvEventPtr = IoEventPtr<IoRecvEvent, IoRecvEventData>;
		using IoSendEventPtr = IoEventPtr<IoSendEvent, IoSendEventData>;
		using IoSendShortEventPtr = IoEventPtr<IoSendEvent, IoSendEventShortData>;

		IoEventObjectPool(std::size_t max_capacity)
			: accept_event_pool{ 1 }
			, send_event_pool{ max_capacity }
			, send_event_data_pool{ max_capacity }
			, send_event_short_data_pool{ max_capacity }
			, recv_event_pool{ max_capacity }
			, recv_event_data_pool{ max_capacity + 1}
		{ }

		auto new_accept_event()
		{
			auto io_buf = recv_event_data_pool.new_object();
			auto io_event = accept_event_pool.new_object(win::ObjectPool<IoRecvEventData>::find_object(io_buf));
			return IoAcceptEventPtr{ std::move(io_event), std::move(io_buf) };
		}

		auto new_recv_event()
		{
			auto io_buf = recv_event_data_pool.new_object();
			auto io_event = recv_event_pool.new_object(win::ObjectPool<IoRecvEventData>::find_object(io_buf));
			return IoRecvEventPtr{ std::move(io_event), std::move(io_buf) };
		}

		auto new_send_event()
		{
			auto io_buf = send_event_data_pool.new_object();
			auto io_event = send_event_pool.new_object(win::ObjectPool<IoSendEventData>::find_object(io_buf));
			return IoSendEventPtr{ std::move(io_event), std::move(io_buf) };
		}

		auto new_send_short_event()
		{
			auto io_buf = send_event_short_data_pool.new_object();
			auto io_event = send_event_pool.new_object(win::ObjectPool<IoSendEventShortData>::find_object(io_buf));
			return IoSendShortEventPtr{ std::move(io_event), std::move(io_buf) };
		}

	private:
		win::ObjectPool<IoAcceptEvent> accept_event_pool;

		win::ObjectPool<IoSendEvent> send_event_pool;
		win::ObjectPool<IoSendEventData> send_event_data_pool;
		win::ObjectPool<IoSendEventShortData> send_event_short_data_pool;

		win::ObjectPool<IoRecvEvent> recv_event_pool;
		win::ObjectPool<IoRecvEventData> recv_event_data_pool;
	};

	using IoEventPool = IoEventObjectPool;

	using IoAcceptEventPtr = IoEventPool::IoAcceptEventPtr;
	using IoRecvEventPtr = IoEventPool::IoRecvEventPtr;
	using IoSendEventPtr = IoEventPool::IoSendEventPtr;
	using IoSendShortEventPtr = IoEventPool::IoSendShortEventPtr;
}