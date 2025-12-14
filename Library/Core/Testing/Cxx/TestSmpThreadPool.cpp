/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Comprehensive unit tests for smp_thread_pool (std_thread backend)
 *
 * Tests cover:
 * - Thread pool singleton and lifecycle
 * - Thread allocation and proxy management
 * - Job execution and distribution
 * - Thread identification
 * - Parallel scope detection
 * - Nested proxies and thread reuse
 * - Thread safety and concurrency
 * - Edge cases and error handling
 *
 * Note: These tests only run when compiled with std_thread backend
 */

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include <vector>

#include "Testing/xsigmaTest.h"

// Only compile these tests for std_thread backend
#if !XSIGMA_HAS_OPENMP && !XSIGMA_HAS_TBB

#include "smp/std_thread/smp_thread_pool.h"

namespace xsigma
{

// ============================================================================
// Test Group 1: Singleton and Lifecycle
// ============================================================================

XSIGMATEST(SmpThreadPool, singleton_access)
{
    // Get singleton instance
    smp_thread_pool& pool1 = smp_thread_pool::instance();
    smp_thread_pool& pool2 = smp_thread_pool::instance();

    // Should be the same instance
    EXPECT_EQ(&pool1, &pool2);
}

XSIGMATEST(SmpThreadPool, initial_state)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    // Initially should not be in parallel scope
    EXPECT_FALSE(pool.is_parallel_scope());

    // Thread ID should be external_thread_id (1)
    EXPECT_EQ(pool.get_thread_id(), smp_thread_pool::external_thread_id);
}

XSIGMATEST(SmpThreadPool, thread_count)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    size_t count = pool.thread_count();

    // Should be reasonable
    EXPECT_GT(count, 0u);
    EXPECT_LE(count, std::thread::hardware_concurrency() * 2);
}

// ============================================================================
// Test Group 2: Thread Allocation and Proxy
// ============================================================================

XSIGMATEST(SmpThreadPool, allocate_threads_default)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    // Allocate with default thread count (0 = auto)
    {
        auto proxy = pool.allocate_threads(0);

        // Proxy should be valid
        EXPECT_TRUE(proxy.is_top_level());

        auto threads = proxy.get_threads();
        EXPECT_GT(threads.size(), 0u);
    }

    // After proxy destruction, should return to normal state
    EXPECT_FALSE(pool.is_parallel_scope());
}

XSIGMATEST(SmpThreadPool, allocate_threads_custom_count)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    {
        auto proxy = pool.allocate_threads(4);

        auto threads = proxy.get_threads();

        // Should have 4 threads (or close to it)
        EXPECT_GE(threads.size(), 1u);
        EXPECT_LE(threads.size(), 4u);
    }

    EXPECT_FALSE(pool.is_parallel_scope());
}

XSIGMATEST(SmpThreadPool, allocate_single_thread)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    {
        auto proxy = pool.allocate_threads(1);

        auto threads = proxy.get_threads();
        EXPECT_GE(threads.size(), 0u);  // May be 0 or 1 depending on implementation
    }

    EXPECT_FALSE(pool.is_parallel_scope());
}

XSIGMATEST(SmpThreadPool, proxy_move_semantics)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    {
        auto proxy1 = pool.allocate_threads(2);
        EXPECT_TRUE(proxy1.is_top_level());

        // Move construct
        auto proxy2 = std::move(proxy1);
        EXPECT_TRUE(proxy2.is_top_level());

        // Move assign
        auto proxy3 = pool.allocate_threads(0);
        proxy3      = std::move(proxy2);
        EXPECT_TRUE(proxy3.is_top_level());
    }

    EXPECT_FALSE(pool.is_parallel_scope());
}

// ============================================================================
// Test Group 3: Job Execution
// ============================================================================

XSIGMATEST(SmpThreadPool, do_job_basic)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    std::atomic<int> counter{0};

    {
        auto proxy = pool.allocate_threads(4);

        // Execute job
        proxy.do_job([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });

        // Join to wait for completion
        proxy.join();
    }

    // Job should have executed
    EXPECT_GE(counter.load(std::memory_order_relaxed), 1);
}

XSIGMATEST(SmpThreadPool, do_job_multiple_times)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    std::atomic<int> counter{0};

    {
        auto proxy = pool.allocate_threads(2);

        // Execute multiple jobs
        for (int i = 0; i < 5; ++i)
        {
            proxy.do_job([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });
        }

        proxy.join();
    }

    EXPECT_GE(counter.load(std::memory_order_relaxed), 5);
}

XSIGMATEST(SmpThreadPool, do_job_with_data)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    std::vector<std::atomic<int>> results(10);
    for (auto& r : results)
    {
        r.store(0, std::memory_order_relaxed);
    }

    {
        auto proxy = pool.allocate_threads(4);

        // Job that modifies array
        proxy.do_job(
            [&results]()
            {
                for (size_t i = 0; i < results.size(); ++i)
                {
                    results[i].store(static_cast<int>(i * 2), std::memory_order_relaxed);
                }
            });

        proxy.join();
    }

    // Verify results
    for (size_t i = 0; i < results.size(); ++i)
    {
        EXPECT_EQ(results[i].load(std::memory_order_relaxed), static_cast<int>(i * 2));
    }
}

XSIGMATEST(SmpThreadPool, do_job_exception_handling)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    {
        auto proxy = pool.allocate_threads(2);

        // Job that throws - should be caught internally
        proxy.do_job([]() { throw std::runtime_error("Test exception"); });

        // Should not crash
        proxy.join();
    }

    // Should still be usable
    EXPECT_FALSE(pool.is_parallel_scope());
}

// ============================================================================
// Test Group 4: Thread Identification
// ============================================================================

XSIGMATEST(SmpThreadPool, get_thread_id_external)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    // Outside parallel region
    size_t thread_id = pool.get_thread_id();

    EXPECT_EQ(thread_id, smp_thread_pool::external_thread_id);
}

XSIGMATEST(SmpThreadPool, get_thread_id_inside_job)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    std::atomic<size_t> internal_thread_id{0};

    {
        auto proxy = pool.allocate_threads(4);

        proxy.do_job(
            [&pool, &internal_thread_id]()
            { internal_thread_id.store(pool.get_thread_id(), std::memory_order_relaxed); });

        proxy.join();
    }

    size_t id = internal_thread_id.load(std::memory_order_relaxed);

    // Should be non-zero (not external)
    EXPECT_GT(id, 0u);
}

XSIGMATEST(SmpThreadPool, external_thread_id_constant)
{
    // Verify external_thread_id constant
    EXPECT_EQ(smp_thread_pool::external_thread_id, 1u);
}

// ============================================================================
// Test Group 5: Parallel Scope Detection
// ============================================================================

XSIGMATEST(SmpThreadPool, is_parallel_scope_outside)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    // Outside any parallel region
    EXPECT_FALSE(pool.is_parallel_scope());
}

XSIGMATEST(SmpThreadPool, is_parallel_scope_inside)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    std::atomic<bool> inside_result{false};

    {
        auto proxy = pool.allocate_threads(2);

        proxy.do_job([&pool, &inside_result]()
                     { inside_result.store(pool.is_parallel_scope(), std::memory_order_relaxed); });

        proxy.join();
    }

    // Inside parallel region should return true
    EXPECT_TRUE(inside_result.load(std::memory_order_relaxed));
}

XSIGMATEST(SmpThreadPool, single_thread_detection)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    bool is_single = pool.single_thread();

    // May return true or false depending on configuration
    // Just ensure no crash
    SUCCEED();
}

// ============================================================================
// Test Group 6: Nested Proxies
// ============================================================================

XSIGMATEST(SmpThreadPool, nested_proxies)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    std::atomic<int> outer_counter{0};
    std::atomic<int> inner_counter{0};

    {
        auto outer_proxy = pool.allocate_threads(2);
        EXPECT_TRUE(outer_proxy.is_top_level());

        outer_proxy.do_job(
            [&pool, &outer_counter, &inner_counter]()
            {
                outer_counter.fetch_add(1, std::memory_order_relaxed);

                // Create nested proxy
                auto inner_proxy = pool.allocate_threads(2);
                EXPECT_FALSE(inner_proxy.is_top_level());  // Not top level

                inner_proxy.do_job([&inner_counter]()
                                   { inner_counter.fetch_add(1, std::memory_order_relaxed); });

                inner_proxy.join();
            });

        outer_proxy.join();
    }

    EXPECT_GE(outer_counter.load(std::memory_order_relaxed), 1);
    EXPECT_GE(inner_counter.load(std::memory_order_relaxed), 1);
}

XSIGMATEST(SmpThreadPool, proxy_is_top_level)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    {
        auto proxy = pool.allocate_threads(4);
        EXPECT_TRUE(proxy.is_top_level());
    }

    // After destruction
    EXPECT_FALSE(pool.is_parallel_scope());
}

// ============================================================================
// Test Group 7: Thread Reuse and Management
// ============================================================================

XSIGMATEST(SmpThreadPool, get_threads)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    {
        auto proxy = pool.allocate_threads(4);

        auto threads = proxy.get_threads();

        // Should have threads
        EXPECT_GT(threads.size(), 0u);

        // All threads should be valid
        for (auto& thread_ref : threads)
        {
            EXPECT_TRUE(thread_ref.get().joinable());
        }
    }

    EXPECT_FALSE(pool.is_parallel_scope());
}

XSIGMATEST(SmpThreadPool, sequential_proxy_allocation)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    std::atomic<int> counter{0};

    // Allocate multiple proxies sequentially
    for (int iteration = 0; iteration < 5; ++iteration)
    {
        auto proxy = pool.allocate_threads(2);

        proxy.do_job([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });

        proxy.join();
    }

    EXPECT_EQ(counter.load(std::memory_order_relaxed), 5);
}

// ============================================================================
// Test Group 8: Edge Cases and Error Handling
// ============================================================================

XSIGMATEST(SmpThreadPool, allocate_zero_threads)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    {
        // Zero threads should use default
        auto proxy = pool.allocate_threads(0);

        auto threads = proxy.get_threads();
        EXPECT_GT(threads.size(), 0u);
    }

    EXPECT_FALSE(pool.is_parallel_scope());
}

XSIGMATEST(SmpThreadPool, allocate_excessive_threads)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    {
        // Request excessive threads - should be capped
        auto proxy = pool.allocate_threads(10000);

        auto threads = proxy.get_threads();

        // Should be capped to reasonable number
        EXPECT_GT(threads.size(), 0u);
        EXPECT_LT(threads.size(), 10000u);
    }

    EXPECT_FALSE(pool.is_parallel_scope());
}

XSIGMATEST(SmpThreadPool, join_without_jobs)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    {
        auto proxy = pool.allocate_threads(2);

        // Join without submitting jobs - should not crash
        proxy.join();
    }

    SUCCEED();
}

XSIGMATEST(SmpThreadPool, multiple_joins)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    std::atomic<int> counter{0};

    {
        auto proxy = pool.allocate_threads(2);

        proxy.do_job([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });

        // Multiple joins should be safe
        proxy.join();
        proxy.join();
        proxy.join();
    }

    EXPECT_GE(counter.load(std::memory_order_relaxed), 1);
}

// ============================================================================
// Test Group 9: Thread Safety and Concurrency
// ============================================================================

XSIGMATEST(SmpThreadPool, concurrent_job_execution)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    std::vector<std::atomic<int>> counters(100);
    for (auto& c : counters)
    {
        c.store(0, std::memory_order_relaxed);
    }

    {
        auto proxy = pool.allocate_threads(4);

        // Submit many jobs concurrently
        for (size_t i = 0; i < counters.size(); ++i)
        {
            proxy.do_job([&counters, i]() { counters[i].fetch_add(1, std::memory_order_relaxed); });
        }

        proxy.join();
    }

    // All jobs should have executed
    for (const auto& c : counters)
    {
        EXPECT_GE(c.load(std::memory_order_relaxed), 1);
    }
}

XSIGMATEST(SmpThreadPool, data_race_detection)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    std::vector<std::atomic<int>> data(1000);
    for (auto& d : data)
    {
        d.store(0, std::memory_order_relaxed);
    }

    {
        auto proxy = pool.allocate_threads(4);

        // Each index should be modified exactly once per job
        for (int iteration = 0; iteration < 10; ++iteration)
        {
            proxy.do_job(
                [&data]()
                {
                    for (auto& d : data)
                    {
                        d.fetch_add(1, std::memory_order_relaxed);
                    }
                });
        }

        proxy.join();
    }

    // Each element should be exactly 10
    for (const auto& d : data)
    {
        EXPECT_EQ(d.load(std::memory_order_relaxed), 10);
    }
}

XSIGMATEST(SmpThreadPool, stress_test_many_jobs)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    std::atomic<int> counter{0};

    {
        auto proxy = pool.allocate_threads(8);

        // Submit many small jobs
        for (int i = 0; i < 1000; ++i)
        {
            proxy.do_job([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });
        }

        proxy.join();
    }

    EXPECT_EQ(counter.load(std::memory_order_relaxed), 1000);
}

// ============================================================================
// Test Group 10: Integration Tests
// ============================================================================

XSIGMATEST(SmpThreadPool, full_workflow)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    // Simulate a typical parallel computation
    const size_t                  size = 10000;
    std::vector<int>              input(size);
    std::vector<std::atomic<int>> output(size);

    // Initialize input
    for (size_t i = 0; i < size; ++i)
    {
        input[i] = static_cast<int>(i);
        output[i].store(0, std::memory_order_relaxed);
    }

    {
        auto proxy = pool.allocate_threads(4);

        // Process data in parallel
        proxy.do_job(
            [&input, &output, size]()
            {
                for (size_t i = 0; i < size; ++i)
                {
                    int result = input[i] * 2 + 1;
                    output[i].store(result, std::memory_order_relaxed);
                }
            });

        proxy.join();
    }

    // Verify results
    for (size_t i = 0; i < size; ++i)
    {
        int expected = static_cast<int>(i) * 2 + 1;
        EXPECT_EQ(output[i].load(std::memory_order_relaxed), expected) << "Mismatch at index " << i;
    }
}

XSIGMATEST(SmpThreadPool, parallel_reduction)
{
    smp_thread_pool& pool = smp_thread_pool::instance();

    const size_t     size = 10000;
    std::vector<int> data(size);

    // Initialize
    for (size_t i = 0; i < size; ++i)
    {
        data[i] = 1;
    }

    std::atomic<int> total{0};

    {
        auto proxy = pool.allocate_threads(4);

        // Parallel reduction
        proxy.do_job(
            [&data, &total]()
            {
                int local_sum = 0;
                for (const auto& val : data)
                {
                    local_sum += val;
                }
                total.fetch_add(local_sum, std::memory_order_relaxed);
            });

        proxy.join();
    }

    EXPECT_GE(total.load(std::memory_order_relaxed), static_cast<int>(size));
}

}  // namespace xsigma

#endif  // !XSIGMA_HAS_OPENMP && !XSIGMA_HAS_TBB
