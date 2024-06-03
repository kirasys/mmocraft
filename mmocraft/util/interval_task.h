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
		std::size_t period;
		std::size_t expired_at;

		using func_type = void (T::*const)();
		func_type func = nullptr;
	};

	template <>
	struct IntervalTask<void>
	{
		std::string tag;
		std::size_t period;
		std::size_t expired_at;

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

		void schedule(std::string_view tag, IntervalTask<T>::func_type func, MilliSecond period)
		{
			interval_tasks.push_back({
				.tag {tag},
				.period = std::size_t(period),
				.expired_at = util::current_monotonic_tick() + std::size_t(period),
				.func = func
			});
		}

		void process_tasks()
		{
			auto current_tick = util::current_monotonic_tick();

			for (IntervalTask<T>& task : interval_tasks)
			{
				if (current_tick < task.expired_at)
					continue;

				try {
					if constexpr (std::is_class_v<T>)
						std::invoke(task.func, *_instance);
					else
						std::invoke(task.func);

					task.expired_at = util::current_monotonic_tick() + task.period;
				}
				catch (...) {
					LOG(error) << "Exception occured at " << task.tag << " (Task deferred)";
					task.expired_at = util::current_monotonic_tick() + (task.period << 2);
				}
			}
		}

	private:
		T* _instance;
		std::vector<IntervalTask<T>> interval_tasks;
	};
}