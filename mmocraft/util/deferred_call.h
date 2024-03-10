#pragma once
#include <utility>
#include <type_traits>

namespace util
{
	template<typename T>
	class deferred_call
	{
	public:
		deferred_call(T&& callable)
			: m_callable(std::forward<T>(callable))
		{
			static_assert(std::is_invocable_v<T>);
		}

		~deferred_call()
		{
			m_callable();
		}
		
		// no move/copy
		deferred_call(deferred_call& dpc) = delete;
		deferred_call& operator=(deferred_call&) = delete;
		deferred_call(deferred_call&& dpc) = delete;
		deferred_call& operator=(deferred_call&&) = delete;
		
	private:
		T m_callable;
	};

	template<typename T>
	using defer = deferred_call<T>;
}

