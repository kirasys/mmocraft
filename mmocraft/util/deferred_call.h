#pragma once
#include <utility>
#include <type_traits>

namespace util
{
	template<typename T>
	class DeferredCall
	{
	public:
		DeferredCall(T&& callable)
			: m_callable(std::forward<T>(callable))
		{
			static_assert(std::is_invocable_v<T>);
		}

		~DeferredCall()
		{
			m_callable();
		}
		
		// no move/copy
		DeferredCall(DeferredCall& dpc) = delete;
		DeferredCall& operator=(DeferredCall&) = delete;
		DeferredCall(DeferredCall&& dpc) = delete;
		DeferredCall& operator=(DeferredCall&&) = delete;
		
	private:
		T m_callable;
	};

	template<typename T>
	using defer = DeferredCall<T>;
}

