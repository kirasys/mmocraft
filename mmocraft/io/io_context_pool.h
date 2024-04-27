#pragma once

#include <cstdlib>

#include "io_context.h"
#include "win/object_pool.h"

namespace io
{
	/// ������Ʈ Ǯ�� ���� IoContext ����ü�� �Ҵ��ϴ� Ŭ����.
	/// IoContext Ÿ�Ը��� ������ Ǯ�� ������� �ʰ�, �ϳ��� Ǯ�� ���ӵ�(Sequential) ������ �Ҵ�. 
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