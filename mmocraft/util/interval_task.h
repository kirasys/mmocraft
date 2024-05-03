#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <functional>
#include <type_traits>
#include <cassert>

#include "time_util.h"
#include "noncopyable.h"
#include "logging/logger.h"

namespace util
{
	template <typename T>
	struct IntervalTask
	{
		std::string tag;
		const Second period;
		std::time_t expired_at;

		using func_type = void (T::*const)();
		func_type func = nullptr;
	};

	template <>
	struct IntervalTask<void>
	{
		std::string tag;
		const Second period;
		std::time_t expired_at;

		using func_type = void (*const)();
		func_type func = nullptr;
	};

	template <typename T>
	class IntervalTaskScheduler : util::NonCopyable, util::NonMovable
	{
	public:
		IntervalTaskScheduler(T* instance = nullptr)
			: _instance{ instance }
		{ 
			if constexpr (std::is_class_v<T>)
				assert(("class type scheduler must have instance.", instance != nullptr));
		}

		void schedule(std::string_view tag, IntervalTask<T>::func_type func, Second period)
		{
			interval_tasks.push_back({
				.tag {tag},
				.period = period,
				.expired_at = current_timestmap() + unsigned(period),
				.func = func
			});
		}

		void process_tasks()
		{
			auto current_time = util::current_timestmap();

			for (IntervalTask<T>& task : interval_tasks)
			{
				if (current_time < task.expired_at)
					continue;

				try {
					if constexpr (std::is_class_v<T>)
						std::invoke(task.func, *_instance);
					else
						std::invoke(task.func);

					task.expired_at = util::current_timestmap() + unsigned(task.period);
				}
				catch (...) {
					logging::cerr() << "Exception occured at " << task.tag << " (Task disabled)";
					task.expired_at = std::numeric_limits<decltype(task.expired_at)>::max();
				}
			}
		}

	private:
		T* _instance;
		std::vector<IntervalTask<T>> interval_tasks;
	};
}