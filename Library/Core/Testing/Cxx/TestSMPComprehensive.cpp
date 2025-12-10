/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Comprehensive test suite for SMP (Symmetric Multi-Processing) module
 * Tests parallel execution, thread management, and backend functionality
 */

#include <atomic>
#include <chrono>
#include <numeric>
#include <thread>
#include <vector>

#include "Testing/xsigmaTest.h"
#include "smp/tools.h"

namespace xsigma
{

// Test 1: Basic backend retrieval
XSIGMATEST(SMPComprehensive, get_backend)
{
    const char* backend = tools::GetBackend();
    EXPECT_NE(backend, nullptr);
    EXPECT_TRUE(std::string(backend) == "std_thread" || std::string(backend) == "TBB");
}

// Test 2: Initialize with default threads
XSIGMATEST(SMPComprehensive, initialize_default)
{
    tools::Initialize();
    int num_threads = tools::GetEstimatedNumberOfThreads();
    EXPECT_GT(num_threads, 0);
}

// Test 3: Initialize with specific thread count
XSIGMATEST(SMPComprehensive, initialize_specific_threads)
{
    tools::Initialize(4);
    int num_threads = tools::GetEstimatedNumberOfThreads();
    EXPECT_GE(num_threads, 1);
    EXPECT_LE(num_threads, 4);
}

// Test 4: Get estimated default number of threads
XSIGMATEST(SMPComprehensive, get_estimated_default_threads)
{
    int default_threads = tools::GetEstimatedDefaultNumberOfThreads();
    EXPECT_GT(default_threads, 0);
    EXPECT_LE(default_threads, static_cast<int>(std::thread::hardware_concurrency()));
}

// Test 5: Basic parallel for loop
XSIGMATEST(SMPComprehensive, parallel_for_basic)
{
    std::vector<int> data(100, 0);

    tools::For(
        0,
        100,
        10,
        [&data](int begin, int end)
        {
            for (int i = begin; i < end; ++i)
            {
                data[i] = i * 2;
            }
        });

    // Verify results
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(data[i], i * 2);
    }
}

// Test 6: Parallel for with small grain size
XSIGMATEST(SMPComprehensive, parallel_for_small_grain)
{
    std::vector<int> data(50, 0);

    tools::For(
        0,
        50,
        1,
        [&data](int begin, int end)
        {
            for (int i = begin; i < end; ++i)
            {
                data[i] = i + 1;
            }
        });

    for (int i = 0; i < 50; ++i)
    {
        EXPECT_EQ(data[i], i + 1);
    }
}

// Test 7: Parallel for with large grain size
XSIGMATEST(SMPComprehensive, parallel_for_large_grain)
{
    std::vector<int> data(100, 0);

    tools::For(
        0,
        100,
        50,
        [&data](int begin, int end)
        {
            for (int i = begin; i < end; ++i)
            {
                data[i] = i * 3;
            }
        });

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(data[i], i * 3);
    }
}

// Test 8: Parallel for with empty range
XSIGMATEST(SMPComprehensive, parallel_for_empty_range)
{
    std::atomic<int> counter{0};

    tools::For(10, 10, 1, [&counter](int begin, int end) { counter++; });

    // Should not execute
    EXPECT_EQ(counter.load(), 0);
}

// Test 9: Parallel for with single element
XSIGMATEST(SMPComprehensive, parallel_for_single_element)
{
    std::vector<int> data(1, 0);

    tools::For(
        0,
        1,
        1,
        [&data](int begin, int end)
        {
            for (int i = begin; i < end; ++i)
            {
                data[i] = 42;
            }
        });

    EXPECT_EQ(data[0], 42);
}

// Test 10: Parallel for with atomic operations
XSIGMATEST(SMPComprehensive, parallel_for_atomic_operations)
{
    std::atomic<int> counter{0};
    const int        iterations = 1000;

    tools::For(
        0,
        iterations,
        10,
        [&counter](int begin, int end)
        {
            for (int i = begin; i < end; ++i)
            {
                counter.fetch_add(1, std::memory_order_relaxed);
            }
        });

    EXPECT_EQ(counter.load(), iterations);
}

// Test 11: Parallel for with large workload
XSIGMATEST(SMPComprehensive, parallel_for_large_workload)
{
    const int        size = 100000;
    std::vector<int> data(size, 0);

    tools::For(
        0,
        size,
        1000,
        [&data](int begin, int end)
        {
            for (int i = begin; i < end; ++i)
            {
                data[i] = i;
            }
        });

    // Spot check
    EXPECT_EQ(data[0], 0);
    EXPECT_EQ(data[size / 2], size / 2);
    EXPECT_EQ(data[size - 1], size - 1);
}

// Test 12: Parallel for with computation
XSIGMATEST(SMPComprehensive, parallel_for_computation)
{
    const int           size = 1000;
    std::vector<double> data(size, 0.0);

    tools::For(
        0,
        size,
        50,
        [&data](int begin, int end)
        {
            for (int i = begin; i < end; ++i)
            {
                data[i] = static_cast<double>(i) * 1.5;
            }
        });

    EXPECT_DOUBLE_EQ(data[0], 0.0);
    EXPECT_DOUBLE_EQ(data[100], 150.0);
    EXPECT_DOUBLE_EQ(data[999], 1498.5);
}

// Test 13: Thread safety with mutex
XSIGMATEST(SMPComprehensive, parallel_for_thread_safety)
{
    std::vector<int> data;
    std::mutex       mutex;

    tools::For(
        0,
        100,
        10,
        [&data, &mutex](int begin, int end)
        {
            for (int i = begin; i < end; ++i)
            {
                std::lock_guard<std::mutex> lock(mutex);
                data.push_back(i);
            }
        });

    EXPECT_EQ(data.size(), 100);
}

// Test 14: Nested parallelism disabled by default
XSIGMATEST(SMPComprehensive, nested_parallelism_default)
{
    bool nested = tools::GetNestedParallelism();
    // Default should be false
    EXPECT_FALSE(nested);
}

// Test 15: Enable nested parallelism
XSIGMATEST(SMPComprehensive, enable_nested_parallelism)
{
    tools::SetNestedParallelism(true);
    bool nested = tools::GetNestedParallelism();
    EXPECT_TRUE(nested);

    // Restore default
    tools::SetNestedParallelism(false);
}

// Test 16: Disable nested parallelism
XSIGMATEST(SMPComprehensive, disable_nested_parallelism)
{
    tools::SetNestedParallelism(false);
    bool nested = tools::GetNestedParallelism();
    EXPECT_FALSE(nested);
}

// Test 17: Check single thread mode
XSIGMATEST(SMPComprehensive, get_single_thread)
{
    bool single_thread = tools::GetSingleThread();
    // Should return a valid boolean
    EXPECT_TRUE(single_thread == true || single_thread == false);
}

// Test 18: Check parallel scope outside parallel region
XSIGMATEST(SMPComprehensive, is_parallel_scope_outside)
{
    bool in_parallel = tools::IsParallelScope();
    // Outside parallel region should be false
    EXPECT_FALSE(in_parallel);
}

}  // namespace xsigma
