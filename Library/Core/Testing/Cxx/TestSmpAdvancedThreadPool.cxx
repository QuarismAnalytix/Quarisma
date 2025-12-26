#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "Testing/xsigmaTest.h"
#include "smp/Advanced/thread_pool.h"

namespace xsigma::detail::smp::Advanced
{

// Test basic thread pool creation and destruction
XSIGMATEST(SmpAdvancedThreadPool, create_and_destroy)
{
    task_thread_pool pool(4);
    EXPECT_EQ(pool.Size(), 4);
}

// Test default thread pool size
XSIGMATEST(SmpAdvancedThreadPool, default_size)
{
    size_t default_size = task_thread_pool_base::DefaultNumThreads();
    EXPECT_GT(default_size, 0);
}

// Test task execution
XSIGMATEST(SmpAdvancedThreadPool, execute_task)
{
    task_thread_pool pool(2);
    std::atomic<int> counter{0};

    pool.Run([&counter]() { ++counter; });

    // Give the thread pool time to execute the task
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(counter.load(), 1);
}

// Test multiple task execution
XSIGMATEST(SmpAdvancedThreadPool, execute_multiple_tasks)
{
    task_thread_pool pool(4);
    std::atomic<int> counter{0};
    const int        num_tasks = 10;

    for (int i = 0; i < num_tasks; ++i)
    {
        pool.Run([&counter]() { ++counter; });
    }

    pool.WaitWorkComplete();
    EXPECT_EQ(counter.load(), num_tasks);
}

// Test wait for work complete
XSIGMATEST(SmpAdvancedThreadPool, wait_work_complete)
{
    task_thread_pool pool(2);
    std::atomic<int> counter{0};

    for (int i = 0; i < 5; ++i)
    {
        pool.Run(
            [&counter]()
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                ++counter;
            });
    }

    pool.WaitWorkComplete();
    EXPECT_EQ(counter.load(), 5);
}

// Test num available threads
XSIGMATEST(SmpAdvancedThreadPool, num_available)
{
    task_thread_pool pool(4);
    size_t           available = pool.NumAvailable();
    EXPECT_EQ(available, 4);
}

// Test in thread pool detection
XSIGMATEST(SmpAdvancedThreadPool, in_thread_pool)
{
    task_thread_pool  pool(2);
    std::atomic<bool> in_pool{false};

    pool.Run([&in_pool, &pool]() { in_pool = pool.InThreadPool(); });

    pool.WaitWorkComplete();
    EXPECT_TRUE(in_pool.load());
}

// Test in thread pool from main thread
XSIGMATEST(SmpAdvancedThreadPool, not_in_thread_pool_main)
{
    task_thread_pool pool(2);
    EXPECT_FALSE(pool.InThreadPool());
}

// Test concurrent task submission
XSIGMATEST(SmpAdvancedThreadPool, concurrent_submission)
{
    task_thread_pool pool(4);
    std::atomic<int> counter{0};
    const int        num_tasks = 20;

    std::vector<std::thread> submitters;
    for (int i = 0; i < 4; ++i)
    {
        submitters.emplace_back(
            [&pool, &counter, num_tasks]()
            {
                for (int j = 0; j < num_tasks / 4; ++j)
                {
                    pool.Run([&counter]() { ++counter; });
                }
            });
    }

    for (auto& t : submitters)
    {
        t.join();
    }

    pool.WaitWorkComplete();
    EXPECT_EQ(counter.load(), num_tasks);
}

// Test task execution continues
XSIGMATEST(SmpAdvancedThreadPool, task_execution_continues)
{
    task_thread_pool pool(2);
    std::atomic<int> counter{0};

    // First task
    pool.Run([&counter]() { ++counter; });

    // Second task
    pool.Run([&counter]() { ++counter; });

    pool.WaitWorkComplete();
    EXPECT_EQ(counter.load(), 2);
}

// Test pool with single thread
XSIGMATEST(SmpAdvancedThreadPool, single_thread_pool)
{
    task_thread_pool pool(1);
    EXPECT_EQ(pool.Size(), 1);
    std::atomic<int> counter{0};

    for (int i = 0; i < 5; ++i)
    {
        pool.Run([&counter]() { ++counter; });
    }

    pool.WaitWorkComplete();
    EXPECT_EQ(counter.load(), 5);
}

// Test pool with many threads
XSIGMATEST(SmpAdvancedThreadPool, many_threads_pool)
{
    task_thread_pool pool(16);
    EXPECT_EQ(pool.Size(), 16);
    std::atomic<int> counter{0};

    for (int i = 0; i < 32; ++i)
    {
        pool.Run([&counter]() { ++counter; });
    }

    pool.WaitWorkComplete();
    EXPECT_EQ(counter.load(), 32);
}

}  // namespace xsigma::detail::smp::Advanced
