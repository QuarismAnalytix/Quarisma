#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "Testing/xsigmaTest.h"
#include "smp_new/core/thread_pool.h"

namespace xsigma::smp_new::core
{

// Test 1: Create and destroy thread pool
XSIGMATEST(SmpNewThreadPool, create_and_destroy)
{
    auto pool = CreateThreadPool(4);
    EXPECT_EQ(pool->Size(), 4);
}

// Test 2: Default thread pool size
XSIGMATEST(SmpNewThreadPool, default_size)
{
    auto pool = CreateThreadPool(-1);
    EXPECT_GT(pool->Size(), 0);
    EXPECT_LE(pool->Size(), std::thread::hardware_concurrency());
}

// Test 3: Single thread pool
XSIGMATEST(SmpNewThreadPool, single_thread)
{
    auto pool = CreateThreadPool(1);
    EXPECT_EQ(pool->Size(), 1);
}

// Test 4: Execute single task
XSIGMATEST(SmpNewThreadPool, execute_task)
{
    auto             pool = CreateThreadPool(2);
    std::atomic<int> counter{0};

    pool->Run([&counter]() { ++counter; });

    pool->WaitWorkComplete();
    EXPECT_EQ(counter.load(), 1);
}

// Test 5: Execute multiple tasks
XSIGMATEST(SmpNewThreadPool, execute_multiple_tasks)
{
    auto             pool = CreateThreadPool(4);
    std::atomic<int> counter{0};
    const int        num_tasks = 10;

    for (int i = 0; i < num_tasks; ++i)
    {
        pool->Run([&counter]() { ++counter; });
    }

    pool->WaitWorkComplete();
    EXPECT_EQ(counter.load(), num_tasks);
}

// Test 6: Wait for work complete
XSIGMATEST(SmpNewThreadPool, wait_work_complete)
{
    auto             pool = CreateThreadPool(2);
    std::atomic<int> counter{0};

    for (int i = 0; i < 5; ++i)
    {
        pool->Run([&counter]() { ++counter; });
    }

    pool->WaitWorkComplete();
    EXPECT_EQ(counter.load(), 5);
}

// Test 7: Num available threads
XSIGMATEST(SmpNewThreadPool, num_available)
{
    auto   pool      = CreateThreadPool(4);
    size_t available = pool->NumAvailable();
    EXPECT_EQ(available, 4);
}

// Test 8: In thread pool detection
XSIGMATEST(SmpNewThreadPool, in_thread_pool)
{
    auto              pool = CreateThreadPool(2);
    std::atomic<bool> in_pool{false};

    pool->Run([&in_pool, &pool]() { in_pool = pool->InThreadPool(); });

    pool->WaitWorkComplete();
    EXPECT_TRUE(in_pool.load());
}

// Test 9: Not in thread pool from main thread
XSIGMATEST(SmpNewThreadPool, not_in_thread_pool_main)
{
    auto pool = CreateThreadPool(2);
    EXPECT_FALSE(pool->InThreadPool());
}

// Test 10: Concurrent task submission
XSIGMATEST(SmpNewThreadPool, concurrent_submission)
{
    auto             pool = CreateThreadPool(4);
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
                    pool->Run([&counter]() { ++counter; });
                }
            });
    }

    for (auto& t : submitters)
    {
        t.join();
    }

    pool->WaitWorkComplete();
    EXPECT_EQ(counter.load(), num_tasks);
}

// Test 11: Large number of tasks
XSIGMATEST(SmpNewThreadPool, large_number_of_tasks)
{
    auto             pool = CreateThreadPool(8);
    std::atomic<int> counter{0};
    const int        num_tasks = 1000;

    for (int i = 0; i < num_tasks; ++i)
    {
        pool->Run([&counter]() { ++counter; });
    }

    pool->WaitWorkComplete();
    EXPECT_EQ(counter.load(), num_tasks);
}

// Test 12: Task with computation
XSIGMATEST(SmpNewThreadPool, task_with_computation)
{
    auto             pool = CreateThreadPool(4);
    std::atomic<int> sum{0};

    for (int i = 0; i < 100; ++i)
    {
        pool->Run([&sum, i]() { sum.fetch_add(i, std::memory_order_relaxed); });
    }

    pool->WaitWorkComplete();
    EXPECT_EQ(sum.load(), 4950);  // Sum of 0..99
}

// Test 13: Thread pool reuse
XSIGMATEST(SmpNewThreadPool, thread_pool_reuse)
{
    auto             pool = CreateThreadPool(2);
    std::atomic<int> counter{0};

    // First batch
    for (int i = 0; i < 5; ++i)
    {
        pool->Run([&counter]() { ++counter; });
    }
    pool->WaitWorkComplete();
    EXPECT_EQ(counter.load(), 5);

    // Second batch
    for (int i = 0; i < 5; ++i)
    {
        pool->Run([&counter]() { ++counter; });
    }
    pool->WaitWorkComplete();
    EXPECT_EQ(counter.load(), 10);
}

// Test 14: Empty task queue
XSIGMATEST(SmpNewThreadPool, empty_task_queue)
{
    auto pool = CreateThreadPool(2);
    pool->WaitWorkComplete();  // Should not hang
    EXPECT_TRUE(true);
}

// Test 15: Thread safety with atomic operations
XSIGMATEST(SmpNewThreadPool, thread_safety_atomic)
{
    auto             pool = CreateThreadPool(4);
    std::atomic<int> counter{0};
    const int        iterations = 1000;  // Reduced from 10000 for faster execution

    for (int i = 0; i < iterations; ++i)
    {
        pool->Run([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });
    }

    pool->WaitWorkComplete();
    EXPECT_EQ(counter.load(), iterations);
}

// Test 16: Multiple thread pools
XSIGMATEST(SmpNewThreadPool, multiple_pools)
{
    auto             pool1 = CreateThreadPool(2);
    auto             pool2 = CreateThreadPool(2);
    std::atomic<int> counter1{0};
    std::atomic<int> counter2{0};

    pool1->Run([&counter1]() { ++counter1; });
    pool2->Run([&counter2]() { ++counter2; });

    pool1->WaitWorkComplete();
    pool2->WaitWorkComplete();

    EXPECT_EQ(counter1.load(), 1);
    EXPECT_EQ(counter2.load(), 1);
}

// Test 17: Task execution order (FIFO)
XSIGMATEST(SmpNewThreadPool, task_execution_order)
{
    auto              pool = CreateThreadPool(1);  // Single thread for deterministic order
    std::vector<int>  results;
    std::mutex        results_mutex;
    std::atomic<bool> ready{false};

    for (int i = 0; i < 10; ++i)
    {
        pool->Run(
            [&results, &results_mutex, i, &ready]()
            {
                while (!ready.load()) {}  // Wait for all tasks to be queued
                std::lock_guard<std::mutex> lock(results_mutex);
                results.push_back(i);
            });
    }

    ready.store(true);
    pool->WaitWorkComplete();

    EXPECT_EQ(results.size(), 10);
    // With single thread, tasks should execute in FIFO order
    for (size_t i = 0; i < results.size(); ++i)
    {
        EXPECT_EQ(results[i], static_cast<int>(i));
    }
}

// Test 18: Default num threads
XSIGMATEST(SmpNewThreadPool, default_num_threads)
{
    size_t default_threads = TaskThreadPoolBase::DefaultNumThreads();
    EXPECT_GT(default_threads, 0);
    EXPECT_LE(default_threads, std::thread::hardware_concurrency());
}

}  // namespace xsigma::smp_new::core
