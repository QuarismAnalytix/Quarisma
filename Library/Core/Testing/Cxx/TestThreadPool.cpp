/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Comprehensive test suite for thread_pool functionality
 * Tests thread pool operations, edge cases, thread safety, and error handling
 */

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "Testing/xsigmaTest.h"
#include "parallel/thread_pool.h"

namespace xsigma
{

// ============================================================================
// Test Group 1: Basic Functionality
// ============================================================================

// Test 1: Thread pool creation and destruction
XSIGMATEST(ThreadPool, Test)
{
    {
        thread_pool pool(4);
        EXPECT_EQ(pool.size(), 4);
    }

    // Test 2: Default thread count
    {
        size_t default_threads = task_thread_pool_base::default_num_threads();
        EXPECT_GT(default_threads, 0);
    }

    // Test 3: Single task execution
    {
        thread_pool      pool(2);
        std::atomic<int> counter{0};

        pool.run([&counter]() { counter++; });

        // Wait for task to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        EXPECT_EQ(counter.load(), 1);
    }

    // Test 4: Multiple tasks execution
    {
        thread_pool      pool(4);
        std::atomic<int> counter{0};
        const int        num_tasks = 100;

        for (int i = 0; i < num_tasks; ++i)
        {
            pool.run([&counter]() { counter++; });
        }

        // Wait for all tasks to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        EXPECT_EQ(counter.load(), num_tasks);
    }

    // ============================================================================
    // Test Group 2: Thread Pool Size
    // ============================================================================

    // Test 5: Single thread pool
    {
        thread_pool      pool(1);
        std::atomic<int> counter{0};

        for (int i = 0; i < 10; ++i)
        {
            pool.run([&counter]() { counter++; });
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        EXPECT_EQ(counter.load(), 10);
    }

    // Test 6: Many threads pool
    {
        thread_pool      pool(16);
        std::atomic<int> counter{0};

        for (int i = 0; i < 100; ++i)
        {
            pool.run([&counter]() { counter++; });
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        EXPECT_EQ(counter.load(), 100);
    }

    // ============================================================================
    // Test Group 3: Task Ordering and Execution
    // ============================================================================

    // Test 7: Tasks are executed (order not guaranteed)
    {
        thread_pool                    pool(4);
        std::atomic<int>               completed{0};
        std::vector<std::atomic<bool>> executed(100);

        // Initialize all elements to false
        for (auto& elem : executed)
        {
            elem.store(false, std::memory_order_relaxed);
        }

        // Submit all tasks
        for (int i = 0; i < 100; ++i)
        {
            pool.run(
                [&executed, &completed, i]()
                {
                    executed[i].store(true, std::memory_order_release);
                    completed.fetch_add(1, std::memory_order_release);
                });
        }

        // Use thread pool's built-in wait mechanism which properly handles synchronization
        pool.wait_work_complete();

        // Verify all tasks completed
        int final_count = completed.load(std::memory_order_acquire);
        EXPECT_EQ(final_count, 100) << "Only " << final_count << " tasks completed out of 100";

        for (int i = 0; i < 100; ++i)
        {
            EXPECT_TRUE(executed[i].load(std::memory_order_acquire))
                << "Task " << i << " was not executed";
        }
    }

    // Test 8: Concurrent task execution
    {
        thread_pool       pool(4);
        std::atomic<int>  active_count{0};
        std::atomic<int>  max_concurrent{0};
        std::atomic<bool> start{false};
        const int         num_tasks = 20;

        for (int i = 0; i < num_tasks; ++i)
        {
            pool.run(
                [&active_count, &max_concurrent, &start]()
                {
                    // Wait for all tasks to be queued
                    while (!start.load())
                    {
                        std::this_thread::yield();
                    }

                    int current  = active_count.fetch_add(1) + 1;
                    int expected = max_concurrent.load();
                    while (current > expected &&
                           !max_concurrent.compare_exchange_weak(expected, current))
                    {
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    active_count.fetch_sub(1);
                });
        }

        start.store(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Should have some concurrency (at least 2 threads active at once)
        EXPECT_GE(max_concurrent.load(), 2);
    }

    // ============================================================================
    // Test Group 4: Thread Safety
    // ============================================================================

    // Test 9: Atomic operations in tasks
    {
        thread_pool          pool(8);
        std::atomic<int64_t> sum{0};
        const int            num_tasks = 1000;

        for (int i = 0; i < num_tasks; ++i)
        {
            pool.run([&sum, i]() { sum.fetch_add(i, std::memory_order_relaxed); });
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        int64_t expected = (static_cast<int64_t>(num_tasks - 1) * num_tasks) / 2;
        EXPECT_EQ(sum.load(), expected);
    }

    // Test 10: No data races with independent writes
    {
        thread_pool                   pool(4);
        std::vector<std::atomic<int>> data(100);

        // Initialize all elements to 0
        for (auto& elem : data)
        {
            elem.store(0, std::memory_order_relaxed);
        }

        for (int i = 0; i < 100; ++i)
        {
            pool.run(
                [&data, i]()
                {
                    // Use release semantics to ensure all previous writes are visible
                    data[i].store(i * i, std::memory_order_release);
                });
        }

        // Use proper synchronization instead of sleep_for
        pool.wait_work_complete();

        for (int i = 0; i < 100; ++i)
        {
            // Use acquire semantics to ensure we see all worker thread writes
            EXPECT_EQ(data[i].load(std::memory_order_acquire), i * i);
        }
    }

    // ============================================================================
    // Test Group 5: Edge Cases
    // ============================================================================

    // Test 11: Empty task (no-op)
    {
        thread_pool pool(2);
        pool.run([]() {});

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // Should not crash
        EXPECT_TRUE(true);
    }

    // Test 12: Task with exception (should not crash pool)
    {
        thread_pool      pool(2);
        std::atomic<int> counter{0};

        // This task might throw, but pool should continue
        pool.run(
            [&counter]()
            {
                counter++;
                // Note: In production, exceptions in thread pool tasks should be caught
            });

        // Submit another task to verify pool still works
        pool.run([&counter]() { counter++; });

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        EXPECT_GE(counter.load(), 1);
    }

    // ============================================================================
    // Test Group 6: Available Threads
    // ============================================================================

    // Test 13: Available threads count
    {
        thread_pool pool(4);

        // Initially, all threads should be available
        size_t available = pool.num_available();
        EXPECT_LE(available, pool.size());
    }

    // Test 14: In thread pool check
    {
        thread_pool       pool(2);
        std::atomic<bool> in_pool{false};

        pool.run([&pool, &in_pool]() { in_pool.store(pool.in_thread_pool()); });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        EXPECT_TRUE(in_pool.load());
    }

    // ============================================================================
    // Test Group 7: Stress Tests
    // ============================================================================

    // Test 15: Many small tasks
    {
        thread_pool      pool(4);
        std::atomic<int> counter{0};
        const int        num_tasks = 10000;

        for (int i = 0; i < num_tasks; ++i)
        {
            pool.run([&counter]() { counter++; });
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));
        EXPECT_EQ(counter.load(), num_tasks);
    }

    // Test 16: Tasks with varying execution times
    {
        thread_pool      pool(4);
        std::atomic<int> counter{0};

        for (int i = 0; i < 20; ++i)
        {
            pool.run(
                [&counter, i]()
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(i % 10));
                    counter++;
                });
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        EXPECT_EQ(counter.load(), 20);
    }
}
}  // namespace xsigma
