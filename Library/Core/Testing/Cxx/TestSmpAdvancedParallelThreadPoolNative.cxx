#include <atomic>
#include <chrono>
#include <thread>

#include "Testing/xsigmaTest.h"
#include "smp/Advanced/parallel_thread_pool_native.h"

namespace xsigma::detail::smp::Advanced
{

// Test setting number of interop threads
XSIGMATEST(SmpAdvancedParallelThreadPoolNative, set_num_interop_threads)
{
    // Note: This test should be run in isolation as it sets global state
    // We test that the function doesn't crash
    set_num_interop_threads(4);
    EXPECT_TRUE(true);
}

// Test getting number of interop threads
XSIGMATEST(SmpAdvancedParallelThreadPoolNative, get_num_interop_threads)
{
    size_t num_threads = get_num_interop_threads();
    EXPECT_GT(num_threads, 0);
}

// Test launch_no_thread_state
XSIGMATEST(SmpAdvancedParallelThreadPoolNative, launch_no_thread_state)
{
    std::atomic<int> counter{0};

    launch_no_thread_state([&counter]() { ++counter; });

    // Give the thread pool time to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(counter.load(), 1);
}

// Test launch function
XSIGMATEST(SmpAdvancedParallelThreadPoolNative, launch)
{
    std::atomic<int> counter{0};

    launch([&counter]() { ++counter; });

    // Give the thread pool time to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(counter.load(), 1);
}

// Test multiple launches
XSIGMATEST(SmpAdvancedParallelThreadPoolNative, multiple_launches)
{
    std::atomic<int> counter{0};
    const int        num_tasks = 10;

    for (int i = 0; i < num_tasks; ++i)
    {
        launch([&counter]() { ++counter; });
    }

    // Give the thread pool time to execute all tasks
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(counter.load(), num_tasks);
}

// Test launch with different task types
XSIGMATEST(SmpAdvancedParallelThreadPoolNative, launch_different_tasks)
{
    std::atomic<int> counter1{0};
    std::atomic<int> counter2{0};

    launch([&counter1]() { ++counter1; });
    launch([&counter2]() { ++counter2; });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(counter1.load(), 1);
    EXPECT_EQ(counter2.load(), 1);
}

// Test launch_no_thread_state with multiple tasks
XSIGMATEST(SmpAdvancedParallelThreadPoolNative, launch_no_thread_state_multiple)
{
    std::atomic<int> counter{0};
    const int        num_tasks = 5;

    for (int i = 0; i < num_tasks; ++i)
    {
        launch_no_thread_state([&counter]() { ++counter; });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(counter.load(), num_tasks);
}

// Test concurrent launches
XSIGMATEST(SmpAdvancedParallelThreadPoolNative, concurrent_launches)
{
    std::atomic<int> counter{0};
    const int        num_tasks = 20;

    std::vector<std::thread> launchers;
    for (int i = 0; i < 4; ++i)
    {
        launchers.emplace_back(
            [&counter, num_tasks]()
            {
                for (int j = 0; j < num_tasks / 4; ++j)
                {
                    launch([&counter]() { ++counter; });
                }
            });
    }

    for (auto& t : launchers)
    {
        t.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(counter.load(), num_tasks);
}

// Test launch with heavy computation
XSIGMATEST(SmpAdvancedParallelThreadPoolNative, launch_heavy_computation)
{
    std::atomic<int> counter{0};

    launch(
        [&counter]()
        {
            // Simulate some work
            int sum = 0;
            for (int i = 0; i < 1000000; ++i)
            {
                sum += i;
            }
            ++counter;
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(counter.load(), 1);
}

// Test launch with exception safety (user responsibility)
XSIGMATEST(SmpAdvancedParallelThreadPoolNative, launch_continues_after_task)
{
    std::atomic<int> counter{0};

    launch([&counter]() { ++counter; });
    launch([&counter]() { ++counter; });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(counter.load(), 2);
}

}  // namespace xsigma::detail::smp::Advanced
