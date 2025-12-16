/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Comprehensive test suite for parallel API functions
 * Tests thread configuration, parallel region detection, and API correctness
 */

#include <atomic>
#include <thread>
#include <vector>

#include "Testing/xsigmaTest.h"
#include "parallel/parallel.h"

namespace xsigma
{

// ============================================================================
// Test Group 1: Thread Number Configuration
// ============================================================================

// Test 1: Get default number of threads
XSIGMATEST(ParallelApi, Test)
{
    {
        int num_threads = get_num_threads();
        EXPECT_GT(num_threads, 0);
    }

    // Test 2: Set and get number of threads (Note: can only be set once before parallel work)
    {
        // Note: In native backend, set_num_threads can only be called once before any parallel work
        // This test just verifies the API exists and doesn't crash
        int current = get_num_threads();
        EXPECT_GT(current, 0);

        // Calling set_num_threads after parallel work has started will log a warning
        // but won't crash
        set_num_threads(current);
        EXPECT_EQ(get_num_threads(), current);
    }

    // Test 4: Thread number in sequential region
    {
        int thread_num = get_thread_num();
        EXPECT_EQ(thread_num, 0);
    }

    // Test 5: Thread number in parallel region
    {
        std::vector<int> thread_nums(100, -1);

        parallel_for(
            0,
            100,
            10,
            [&thread_nums](int64_t begin, int64_t end)
            {
                int tid = get_thread_num();
                for (int64_t i = begin; i < end; ++i)
                {
                    thread_nums[i] = tid;
                }
            });

        // Verify all thread numbers are valid (>= 0)
        for (int i = 0; i < 100; ++i)
        {
            EXPECT_GE(thread_nums[i], 0);
        }
    }

    // ============================================================================
    // Test Group 2: Parallel Region Detection
    // ============================================================================

    // Test 6: Not in parallel region initially
    {
        EXPECT_FALSE(in_parallel_region());
    }

    // Test 7: In parallel region during parallel_for
    {
        std::atomic<bool> was_in_parallel{false};

        parallel_for(
            0,
            10,
            5,
            [&was_in_parallel](int64_t /*begin*/, int64_t /*end*/)
            {
                if (in_parallel_region())
                {
                    was_in_parallel.store(true);
                }
            });

        EXPECT_TRUE(was_in_parallel.load());
    }

    // Test 8: Not in parallel region after parallel_for
    {
        parallel_for(0, 10, 5, [](int64_t /*begin*/, int64_t /*end*/) {});

        EXPECT_FALSE(in_parallel_region());
    }

    // ============================================================================
    // Test Group 3: Inter-op Thread Configuration
    // ============================================================================

    // Test 9: Get default inter-op threads
    {
        size_t num_threads = get_num_interop_threads();
        EXPECT_GT(num_threads, 0);
    }

    // Test 10: Set inter-op threads
    {
        // Note: This test may fail if inter-op threads are already consumed
        // In production code, set_num_interop_threads should be called before any parallel work
        size_t original = get_num_interop_threads();

        // Try to set (may not work if already initialized)
        set_num_interop_threads(4);

        // Just verify we can get the value
        size_t current = get_num_interop_threads();
        EXPECT_GT(current, 0);
    }

    // ============================================================================
    // Test Group 4: Launch Functions
    // ============================================================================

    // Test 11: Launch simple task
    {
        std::atomic<bool> executed{false};

        launch([&executed]() { executed.store(true); });

        // Wait for task to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        EXPECT_TRUE(executed.load());
    }

    // Test 12: Launch multiple tasks
    {
        std::atomic<int> counter{0};

        for (int i = 0; i < 10; ++i)
        {
            launch([&counter]() { counter++; });
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        EXPECT_EQ(counter.load(), 10);
    }

    // Test 13: Intraop launch
    {
        std::atomic<bool> executed{false};

        intraop_launch([&executed]() { executed.store(true); });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        EXPECT_TRUE(executed.load());
    }

    // Test 14: Intraop default num threads
    {
        int num_threads = intraop_default_num_threads();
        EXPECT_GT(num_threads, 0);
    }

    // ============================================================================
    // Test Group 5: Parallel Info
    // ============================================================================

    // Test 15: Get parallel info string
    {
        std::string info = get_parallel_info();
        EXPECT_FALSE(info.empty());
    }

    // ============================================================================
    // Test Group 6: Init Functions
    // ============================================================================

    // Test 16: Init num threads (should not crash)
    {
        init_num_threads();
        // Should not crash
        EXPECT_TRUE(true);
    }

    // ============================================================================
    // Test Group 7: Edge Cases
    // ============================================================================

    // Test 17: Verify thread count is reasonable
    {
        int current = get_num_threads();
        EXPECT_GT(current, 0);
        EXPECT_LE(current, 256);  // Reasonable upper bound
    }

    // ============================================================================
    // Test Group 8: Divup Utility
    // ============================================================================

    // Test 19: Divup basic functionality
    {
        EXPECT_EQ(divup(10, 3), 4);
        EXPECT_EQ(divup(9, 3), 3);
        EXPECT_EQ(divup(100, 10), 10);
        EXPECT_EQ(divup(101, 10), 11);
    }

    // Test 20: Divup edge cases
    {
        EXPECT_EQ(divup(0, 1), 0);
        EXPECT_EQ(divup(1, 1), 1);
        EXPECT_EQ(divup(1, 10), 1);
        EXPECT_EQ(divup(10, 1), 10);
    }

    // ============================================================================
    // Test Group 9: Thread Safety
    // ============================================================================

    // Test 21: Concurrent get_num_threads calls
    {
        std::vector<std::thread> threads;
        std::atomic<bool>        all_same{true};
        int                      first_value = get_num_threads();

        for (int i = 0; i < 10; ++i)
        {
            threads.emplace_back(
                [&all_same, first_value]()
                {
                    int value = get_num_threads();
                    if (value != first_value)
                    {
                        all_same.store(false);
                    }
                });
        }

        for (auto& t : threads)
        {
            t.join();
        }

        EXPECT_TRUE(all_same.load());
    }

    // Test 22: Concurrent parallel_for calls
    {
        std::vector<std::thread> threads;
        std::atomic<int>         total_sum{0};

        for (int t = 0; t < 4; ++t)
        {
            threads.emplace_back(
                [&total_sum]()
                {
                    std::atomic<int> local_sum{0};
                    parallel_for(
                        0,
                        100,
                        10,
                        [&local_sum](int64_t begin, int64_t end)
                        {
                            for (int64_t i = begin; i < end; ++i)
                            {
                                local_sum.fetch_add(static_cast<int>(i));
                            }
                        });
                    total_sum.fetch_add(local_sum.load());
                });
        }

        for (auto& t : threads)
        {
            t.join();
        }

        int expected_per_thread = (99 * 100) / 2;
        int expected_total      = expected_per_thread * 4;
        EXPECT_EQ(total_sum.load(), expected_total);
    }
}
}  // namespace xsigma
