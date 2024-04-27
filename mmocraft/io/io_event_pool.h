#pragma once

#include <cstdlib>

#include "io_event.h"
#include "win/object_pool.h"

namespace io
{
	/// 오브젝트 풀을 통해 IoContext 구조체를 할당하는 클래스.
	/// IoContext 타입마다 별개의 풀을 사용하지 않고, 하나의 풀로 연속된(Sequential) 공간에 할당. 
	class IoEventSequentialPool
	{	
	public:
		IoEventSequentialPool(std::size_t max_capacity)
			: m_pool{max_capacity}
		{ }

		template <typename T>
		auto new_event()
		{
			return static_cast<T*>(m_pool.new_object_raw());
		}

		template <typename T>
		void delete_event(T* ctx)
		{
			m_pool.free_object(static_cast<IoEvent*>(ctx));
		}

	private:
		struct IoEventSpan : IoEvent
		{
			union OperationContext
			{
				IoAcceptEvent ev_accept;
				IoRecvEvent ev_recv;
				IoSendEvent ec_send;
			} ev_operation;
		};

		win::ObjectPool<IoEvent, sizeof(IoEventSpan)> m_pool;
	};

	// default IoContextPool class
	using IoEventPool = IoEventSequentialPool;
}