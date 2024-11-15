#include "pch.h"

#include "io/async_task.h"
#include "util/time_util.h"

io::AsyncTask<int> co_return_value(int value)
{
    co_return value;
}

TEST(async_task, event_data_manipulation_properly)
{
    int ret = 0;
    auto get_result = [&ret]() -> io::DetachedTask {
        ret = co_await co_return_value(3);
        //ret = 5;
    };

    get_result();

    EXPECT_EQ(ret, 3);
}