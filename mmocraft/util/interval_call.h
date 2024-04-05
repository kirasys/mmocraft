#pragma once
#include <vector>
#include <functional>
#include <type_traits>
#include <cassert>

#include "time_util.h"
#include "noncopyable.h"

namespace util
{
	template <typename T>
	struct IntervalCall
	{
		const Second period;
		std::time_t expired_at;

		using func_type = void (T::*const)();
		func_type func = nullptr;
	};

	template <>
	struct IntervalCall<void>
	{
		const Second period;
		std::time_t expired_at;

		using func_type = void (*const)();
		func_type func = nullptr;
	};

	template <typename T>
	class IntervalCallScheduler : util::NonCopyable, util::NonMovable
	{
	public:
		IntervalCallScheduler(T* instance = nullptr)
			: m_instance(instance)
		{ 
			if constexpr (std::is_class_v<T>)
				assert(("class type scheduler must have instance.", instance != nullptr));
		}

		void schedule(IntervalCall<T>::func_type func, Second period)
		{
			m_intervals.push_back({
				.period = period,
				.expired_at = current_timestmap() + unsigned(period),
				.func = func
			});
		}

		void invoke_expired_call()
		{
			auto current_time = util::current_timestmap();

			for (auto& interval : m_intervals)
			{
				if (current_time < interval.expired_at)
					continue;

				if constexpr (std::is_class_v<T>)
					std::invoke(interval.func, *m_instance);
				else
					std::invoke(interval.func);

				interval.expired_at = util::current_timestmap() + unsigned(interval.period);
			}
		}

	private:
		T* m_instance;
		std::vector<IntervalCall<T>> m_intervals;
	};
}