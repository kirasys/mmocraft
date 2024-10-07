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
    namespace interval_task_tag_id
    {
        enum value
        {
            invalid,

            // Tcp server task
            clean_connection,
            clean_player,

            // Interval server task
            announce_server,

            // Size of enum.
            count
        };
    }
    

    template <typename T>
    struct IntervalTask
    {
        interval_task_tag_id::value tag;
        std::size_t period;
        std::size_t expired_at;

        using func_type = void (T::*const)();
        func_type func = nullptr;
    };

    template <>
    struct IntervalTask<void>
    {
        interval_task_tag_id::value tag;
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
            else
                assert(("static scheduler must not have instance.", instance == nullptr));
        }

        void schedule(util::interval_task_tag_id::value tag, IntervalTask<T>::func_type func, MilliSecond period)
        {
            interval_tasks.push_back({
                .tag {tag},
                .period = std::size_t(period),
                .expired_at = util::current_monotonic_tick() + std::size_t(period),
                .func = func
            });
        }

        void process_tasks(util::interval_task_tag_id::value tag = interval_task_tag_id::invalid)
        {
            for (IntervalTask<T>& task : interval_tasks)
            {
                if (task.tag != interval_task_tag_id::invalid && task.tag != tag)
                    continue;

                if (util::current_monotonic_tick() < task.expired_at)
                    continue;

                invoke_task(task);
            }
        }

    private:

        void invoke_task(IntervalTask<T>& task)
        {
            try {
                if constexpr (std::is_class_v<T>)
                    std::invoke(task.func, *_instance);
                else
                    std::invoke(task.func);

                task.expired_at = util::current_monotonic_tick() + task.period;
            }
            catch (...) {
                LOG(error) << "Exception occured at " << int(task.tag) << " (Task deferred)";
                task.expired_at = util::current_monotonic_tick() + (task.period << 2);
            }
        }

        T* _instance;
        std::vector<IntervalTask<T>> interval_tasks;
    };
}