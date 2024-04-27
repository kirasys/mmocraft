#pragma once

#include <cstdlib>

#include "io_event.h"
#include "win/object_pool.h"

namespace io
{
	/// ������Ʈ Ǯ�� ���� IoContext ����ü�� �Ҵ��ϴ� Ŭ����.
	/// IoContext Ÿ�Ը��� ������ Ǯ�� ������� �ʰ�, �ϳ��� Ǯ�� ���ӵ�(Sequential) ������ �Ҵ�. 
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