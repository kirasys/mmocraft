#pragma once

#include <cstdlib>

#include "io_context.h"
#include "win/object_pool.h"

namespace io
{
	/// 오브젝트 풀을 통해 IoContext 구조체를 할당하는 클래스.
	/// IoContext 타입마다 별개의 풀을 사용하지 않고, 하나의 풀로 연속된(Sequential) 공간에 할당. 
	class IoContextSequentialPool
	{	
	public:
		IoContextSequentialPool(std::size_t max_capacity)
			: m_pool{max_capacity}
		{ }

		template <typename T>
		auto new_context()
		{
			return static_cast<T*>(m_pool.new_object_raw());
		}

		template <typename T>
		void delete_context(T* ctx)
		{
			m_pool.free_object(static_cast<IoContext*>(ctx));
		}

	private:
		struct IoContextSpan : IoContext
		{
			union OperationContext
			{
				IoAcceptContext accept_ctx;
				IoRecvContext recv_ctx;
				IoSendContext send_ctx;
			} operation_ctx;
		};

		win::ObjectPool<struct IoContext, sizeof(IoContextSpan)> m_pool;
	};

	// default IoContextPool class
	using IoContextPool = IoContextSequentialPool;
}