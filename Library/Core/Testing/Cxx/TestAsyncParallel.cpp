/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Comprehensive test suite for async_parallel_for and async_parallel_reduce
 * Tests asynchronous parallel operations, concurrency, error handling, and correctness
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <limits>
#include <numeric>
#include <thread>
#include <vector>

#include "Testing/xsigmaTest.h"
#include "parallel/parallel.h"

namespace xsigma
{

// ============================================================================
// Test Group 1: Basic async_parallel_for Functionality
// ============================================================================

// Test 1: Basic async_parallel_for with simple range
XSIGMATEST(AsyncParallelFor, basic_async_operation)
{
    std::vector<int> data(100, 0);

    auto handle = async_parallel_for(
        0,
        100,
        10,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] = static_cast<int>(i * 2);
            }
        });

    // Verify handle is valid
    EXPECT_TRUE(handle.valid());

    // Wait for completion
    handle.wait();

    // Check for errors
    EXPECT_FALSE(handle.has_error()) << "Error: " << handle.get_error();

    // Verify results
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(data[i], i * 2);
    }
}

// Test 2: Verify async operation returns immediately
XSIGMATEST(AsyncParallelFor, returns_immediately)
{
    std::vector<int> data(1000000, 0);

    auto start = std::chrono::high_resolution_clock::now();

    auto handle = async_parallel_for(
        0,
        1000000,
        10000,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                // Simulate work
                for (int j = 0; j < 100; ++j)
                {
                    data[i] += j;
                }
            }
        });

    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                       std::chrono::high_resolution_clock::now() - start)
                       .count();

    // Launch should return quickly (< 1ms = 1000 μs)
    EXPECT_LT(elapsed, 1000) << "Launch took " << elapsed << " μs (should be < 1000 μs)";

    // Wait for actual work to complete
    handle.wait();
    EXPECT_FALSE(handle.has_error());
}

// Test 3: Multiple concurrent async operations
XSIGMATEST(AsyncParallelFor, multiple_concurrent_operations)
{
    const size_t     N = 1000;
    std::vector<int> data1(N, 0);
    std::vector<int> data2(N, 0);
    std::vector<int> data3(N, 0);

    // Launch three concurrent operations
    auto handle1 = async_parallel_for(
        0,
        static_cast<int64_t>(N),
        100,
        [&data1](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data1[i] = static_cast<int>(i * 1);
            }
        });

    auto handle2 = async_parallel_for(
        0,
        static_cast<int64_t>(N),
        100,
        [&data2](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data2[i] = static_cast<int>(i * 2);
            }
        });

    auto handle3 = async_parallel_for(
        0,
        static_cast<int64_t>(N),
        100,
        [&data3](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data3[i] = static_cast<int>(i * 3);
            }
        });

    // Wait for all operations
    handle1.wait();
    handle2.wait();
    handle3.wait();

    // Check no errors
    EXPECT_FALSE(handle1.has_error());
    EXPECT_FALSE(handle2.has_error());
    EXPECT_FALSE(handle3.has_error());

    // Verify results
    for (size_t i = 0; i < N; ++i)
    {
        EXPECT_EQ(data1[i], static_cast<int>(i * 1));
        EXPECT_EQ(data2[i], static_cast<int>(i * 2));
        EXPECT_EQ(data3[i], static_cast<int>(i * 3));
    }
}

// Test 4: is_ready() polling
XSIGMATEST(AsyncParallelFor, is_ready_polling)
{
    std::vector<int> data(10000, 0);

    auto handle = async_parallel_for(
        0,
        10000,
        1000,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] = static_cast<int>(i);
            }
        });

    // Initially may or may not be ready (depends on scheduling)
    // Poll until ready
    while (!handle.is_ready())
    {
        std::this_thread::yield();
    }

    // Should be ready now
    EXPECT_TRUE(handle.is_ready());
    EXPECT_FALSE(handle.has_error());

    // Verify results
    for (int i = 0; i < 10000; ++i)
    {
        EXPECT_EQ(data[i], i);
    }
}

// Test 5: wait_for() with timeout
XSIGMATEST(AsyncParallelFor, wait_for_timeout)
{
    std::vector<int> data(1000, 0);

    auto handle = async_parallel_for(
        0,
        1000,
        100,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] = static_cast<int>(i);
            }
        });

    // Wait with generous timeout (should succeed)
    bool completed = handle.wait_for(5000);  // 5 seconds
    EXPECT_TRUE(completed) << "Operation should complete within 5 seconds";
    EXPECT_FALSE(handle.has_error());
}

// ============================================================================
// Test Group 2: Edge Cases for async_parallel_for
// ============================================================================

// Test 6: Empty range
XSIGMATEST(AsyncParallelFor, empty_range)
{
    std::atomic<int> counter{0};

    auto handle =
        async_parallel_for(0, 0, 10, [&counter](int64_t /*begin*/, int64_t /*end*/) { ++counter; });

    handle.wait();
    EXPECT_FALSE(handle.has_error());
    EXPECT_EQ(counter.load(), 0);
}

// Test 7: Single element
XSIGMATEST(AsyncParallelFor, single_element)
{
    std::vector<int> data(1, 0);

    auto handle = async_parallel_for(
        0,
        1,
        10,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] = 42;
            }
        });

    handle.wait();
    EXPECT_FALSE(handle.has_error());
    EXPECT_EQ(data[0], 42);
}

// Test 8: Very large range
XSIGMATEST(AsyncParallelFor, large_range)
{
    const size_t                   N = 10000000;  // 10 million
    std::vector<std::atomic<bool>> processed(N);

    for (auto& elem : processed)
    {
        elem.store(false, std::memory_order_relaxed);
    }

    auto handle = async_parallel_for(
        0,
        static_cast<int64_t>(N),
        100000,
        [&processed](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                processed[i].store(true, std::memory_order_release);
            }
        });

    handle.wait();
    EXPECT_FALSE(handle.has_error());

    // Verify all elements were processed
    for (size_t i = 0; i < N; ++i)
    {
        EXPECT_TRUE(processed[i].load(std::memory_order_acquire))
            << "Element " << i << " was not processed";
    }
}

// ============================================================================
// Test Group 3: Thread Safety for async_parallel_for
// ============================================================================

// Test 9: Thread safety with atomic operations
XSIGMATEST(AsyncParallelFor, thread_safety_atomic)
{
    std::atomic<int64_t> sum{0};
    const int64_t        N = 100000;

    auto handle = async_parallel_for(
        0,
        N,
        10000,
        [&sum](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                sum.fetch_add(1, std::memory_order_relaxed);
            }
        });

    handle.wait();
    EXPECT_FALSE(handle.has_error());
    EXPECT_EQ(sum.load(), N);
}

// Test 10: Multiple async operations with shared counter
XSIGMATEST(AsyncParallelFor, concurrent_shared_counter)
{
    std::atomic<int> counter{0};
    const int        ops_per_handle = 1000;

    std::vector<async_handle<void>> handles;

    // Launch 10 concurrent operations
    for (int op = 0; op < 10; ++op)
    {
        handles.emplace_back(async_parallel_for(
            0,
            ops_per_handle,
            100,
            [&counter](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    counter.fetch_add(1, std::memory_order_relaxed);
                }
            }));
    }

    // Wait for all operations
    for (auto& handle : handles)
    {
        handle.wait();
        EXPECT_FALSE(handle.has_error());
    }

    EXPECT_EQ(counter.load(), 10 * ops_per_handle);
}

// ============================================================================
// Test Group 4: Basic async_parallel_reduce Functionality
// ============================================================================

// Test 11: Basic async sum reduction
XSIGMATEST(AsyncParallelReduce, basic_sum_reduction)
{
    std::vector<int> data(1000, 1);  // All elements are 1

    auto handle = async_parallel_reduce(
        0,
        static_cast<int64_t>(data.size()),
        250,
        0,
        [&data](int64_t begin, int64_t end, int identity)
        {
            int partial_sum = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                partial_sum += data[i];
            }
            return partial_sum;
        },
        [](int a, int b) { return a + b; });

    // Verify handle is valid
    EXPECT_TRUE(handle.valid());

    // Get result (blocks until ready)
    int sum = handle.get();

    // Check no errors
    EXPECT_FALSE(handle.has_error()) << "Error: " << handle.get_error();

    // Verify result
    EXPECT_EQ(sum, 1000);
}

// Test 12: Max reduction
XSIGMATEST(AsyncParallelReduce, max_reduction)
{
    std::vector<int> data(10000);
    for (size_t i = 0; i < data.size(); ++i)
    {
        data[i] = static_cast<int>(i);
    }

    auto handle = async_parallel_reduce(
        0,
        static_cast<int64_t>(data.size()),
        1000,
        std::numeric_limits<int>::lowest(),
        [&data](int64_t begin, int64_t end, int identity)
        {
            int partial_max = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                partial_max = std::max(partial_max, data[i]);
            }
            return partial_max;
        },
        [](int a, int b) { return std::max(a, b); });

    int max_val = handle.get();
    EXPECT_FALSE(handle.has_error());
    EXPECT_EQ(max_val, 9999);
}

// Test 13: Min reduction
XSIGMATEST(AsyncParallelReduce, min_reduction)
{
    std::vector<int> data(10000);
    for (size_t i = 0; i < data.size(); ++i)
    {
        data[i] = static_cast<int>(i + 1);
    }

    auto handle = async_parallel_reduce(
        0,
        static_cast<int64_t>(data.size()),
        1000,
        std::numeric_limits<int>::max(),
        [&data](int64_t begin, int64_t end, int identity)
        {
            int partial_min = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                partial_min = std::min(partial_min, data[i]);
            }
            return partial_min;
        },
        [](int a, int b) { return std::min(a, b); });

    int min_val = handle.get();
    EXPECT_FALSE(handle.has_error());
    EXPECT_EQ(min_val, 1);
}

// Test 14: Product reduction
XSIGMATEST(AsyncParallelReduce, product_reduction)
{
    std::vector<int> data = {2, 3, 4, 5};

    auto handle = async_parallel_reduce(
        0,
        static_cast<int64_t>(data.size()),
        1,
        1,
        [&data](int64_t begin, int64_t end, int identity)
        {
            int partial_product = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                partial_product *= data[i];
            }
            return partial_product;
        },
        [](int a, int b) { return a * b; });

    int product = handle.get();
    EXPECT_FALSE(handle.has_error());
    EXPECT_EQ(product, 2 * 3 * 4 * 5);
}

// ============================================================================
// Test Group 5: Multiple Concurrent Reductions
// ============================================================================

// Test 15: Concurrent sum, max, min reductions
XSIGMATEST(AsyncParallelReduce, concurrent_statistics)
{
    std::vector<double> data(100000);
    for (size_t i = 0; i < data.size(); ++i)
    {
        data[i] = static_cast<double>(i % 1000);
    }

    // Launch sum reduction
    auto sum_handle = async_parallel_reduce(
        0,
        static_cast<int64_t>(data.size()),
        10000,
        0.0,
        [&data](int64_t begin, int64_t end, double identity)
        {
            double partial_sum = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                partial_sum += data[i];
            }
            return partial_sum;
        },
        [](double a, double b) { return a + b; });

    // Launch max reduction
    auto max_handle = async_parallel_reduce(
        0,
        static_cast<int64_t>(data.size()),
        10000,
        std::numeric_limits<double>::lowest(),
        [&data](int64_t begin, int64_t end, double identity)
        {
            double partial_max = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                partial_max = std::max(partial_max, data[i]);
            }
            return partial_max;
        },
        [](double a, double b) { return std::max(a, b); });

    // Launch min reduction
    auto min_handle = async_parallel_reduce(
        0,
        static_cast<int64_t>(data.size()),
        10000,
        std::numeric_limits<double>::max(),
        [&data](int64_t begin, int64_t end, double identity)
        {
            double partial_min = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                partial_min = std::min(partial_min, data[i]);
            }
            return partial_min;
        },
        [](double a, double b) { return std::min(a, b); });

    // Get results (all run concurrently)
    double sum     = sum_handle.get();
    double max_val = max_handle.get();
    double min_val = min_handle.get();

    // Check no errors
    EXPECT_FALSE(sum_handle.has_error());
    EXPECT_FALSE(max_handle.has_error());
    EXPECT_FALSE(min_handle.has_error());

    // Verify results
    double expected_sum = 0.0;
    for (const auto& val : data)
    {
        expected_sum += val;
    }
    EXPECT_DOUBLE_EQ(sum, expected_sum);
    EXPECT_DOUBLE_EQ(max_val, 999.0);
    EXPECT_DOUBLE_EQ(min_val, 0.0);
}

// ============================================================================
// Test Group 6: Edge Cases for async_parallel_reduce
// ============================================================================

// Test 16: Empty range returns identity
XSIGMATEST(AsyncParallelReduce, empty_range_returns_identity)
{
    auto handle = async_parallel_reduce(
        0,
        0,
        10,
        42,
        [](int64_t /*begin*/, int64_t /*end*/, int identity) { return identity * 2; },
        [](int a, int b) { return a + b; });

    int result = handle.get();
    EXPECT_FALSE(handle.has_error());
    EXPECT_EQ(result, 42);
}

// Test 17: Single element
XSIGMATEST(AsyncParallelReduce, single_element)
{
    std::vector<int> data = {100};

    auto handle = async_parallel_reduce(
        0,
        1,
        10,
        0,
        [&data](int64_t begin, int64_t end, int identity)
        {
            int sum = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                sum += data[i];
            }
            return sum;
        },
        [](int a, int b) { return a + b; });

    int result = handle.get();
    EXPECT_FALSE(handle.has_error());
    EXPECT_EQ(result, 100);
}

// Test 18: Large reduction
XSIGMATEST(AsyncParallelReduce, large_reduction)
{
    const size_t     N = 10000000;  // 10 million
    std::vector<int> data(N, 1);

    auto handle = async_parallel_reduce(
        0,
        static_cast<int64_t>(N),
        100000,
        0LL,
        [&data](int64_t begin, int64_t end, int64_t identity)
        {
            int64_t sum = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                sum += data[i];
            }
            return sum;
        },
        [](int64_t a, int64_t b) { return a + b; });

    int64_t result = handle.get();
    EXPECT_FALSE(handle.has_error());
    EXPECT_EQ(result, static_cast<int64_t>(N));
}

// ============================================================================
// Test Group 7: Correctness and Determinism
// ============================================================================

// Test 19: Multiple runs produce same result
XSIGMATEST(AsyncParallelReduce, deterministic_results)
{
    std::vector<int> data(10000);
    for (size_t i = 0; i < data.size(); ++i)
    {
        data[i] = static_cast<int>(i);
    }

    // Run reduction multiple times
    std::vector<int64_t> results;
    for (int run = 0; run < 5; ++run)
    {
        auto handle = async_parallel_reduce(
            0,
            static_cast<int64_t>(data.size()),
            1000,
            0LL,
            [&data](int64_t begin, int64_t end, int64_t identity)
            {
                int64_t sum = identity;
                for (int64_t i = begin; i < end; ++i)
                {
                    sum += data[i];
                }
                return sum;
            },
            [](int64_t a, int64_t b) { return a + b; });

        results.push_back(handle.get());
        EXPECT_FALSE(handle.has_error());
    }

    // All results should be the same
    int64_t expected = results[0];
    for (size_t i = 1; i < results.size(); ++i)
    {
        EXPECT_EQ(results[i], expected) << "Run " << i << " produced different result";
    }
}

// Test 20: Verify result correctness against sequential computation
XSIGMATEST(AsyncParallelReduce, correctness_vs_sequential)
{
    std::vector<double> data(50000);
    for (size_t i = 0; i < data.size(); ++i)
    {
        data[i] = static_cast<double>(i) * 0.1;
    }

    // Async parallel reduction
    auto handle = async_parallel_reduce(
        0,
        static_cast<int64_t>(data.size()),
        5000,
        0.0,
        [&data](int64_t begin, int64_t end, double identity)
        {
            double sum = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                sum += data[i];
            }
            return sum;
        },
        [](double a, double b) { return a + b; });

    double async_result = handle.get();
    EXPECT_FALSE(handle.has_error());

    // Sequential computation
    double sequential_result = 0.0;
    for (const auto& val : data)
    {
        sequential_result += val;
    }

    // Results should match (within floating point tolerance)
    EXPECT_NEAR(async_result, sequential_result, 1e-6);
}

// ============================================================================
// Test Group 8: Error Handling
// ============================================================================

// Test 21: Handle validity
XSIGMATEST(AsyncHandle, handle_validity)
{
    // Valid handle
    auto valid_handle = async_parallel_for(0, 100, 10, [](int64_t, int64_t) {});
    EXPECT_TRUE(valid_handle.valid());

    // Default-constructed handle (invalid)
    async_handle<void> invalid_handle;
    EXPECT_FALSE(invalid_handle.valid());
    EXPECT_FALSE(invalid_handle.is_ready());
    EXPECT_FALSE(invalid_handle.has_error());
}

// Test 22: Move semantics
XSIGMATEST(AsyncHandle, move_semantics)
{
    std::vector<int> data(100, 0);

    auto handle1 = async_parallel_for(
        0,
        100,
        10,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] = static_cast<int>(i);
            }
        });

    EXPECT_TRUE(handle1.valid());

    // Move construct
    auto handle2 = std::move(handle1);
    EXPECT_TRUE(handle2.valid());

    // Move assign
    async_handle<void> handle3;
    handle3 = std::move(handle2);
    EXPECT_TRUE(handle3.valid());

    // Wait on moved handle
    handle3.wait();
    EXPECT_FALSE(handle3.has_error());

    // Verify results
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(data[i], i);
    }
}

// Test 23: Multiple waits on same handle
XSIGMATEST(AsyncHandle, multiple_waits)
{
    std::vector<int> data(100, 0);

    auto handle = async_parallel_for(
        0,
        100,
        10,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] = 42;
            }
        });

    // First wait
    handle.wait();
    EXPECT_TRUE(handle.is_ready());
    EXPECT_FALSE(handle.has_error());

    // Second wait (should return immediately)
    handle.wait();
    EXPECT_TRUE(handle.is_ready());
    EXPECT_FALSE(handle.has_error());

    // Third wait
    handle.wait();
    EXPECT_TRUE(handle.is_ready());
}

// ============================================================================
// Test Group 9: Integration Tests
// ============================================================================

// Test 24: Mix of async_parallel_for and async_parallel_reduce
XSIGMATEST(AsyncParallel, mixed_operations)
{
    const size_t     N = 10000;
    std::vector<int> data1(N, 0);
    std::vector<int> data2(N, 0);

    // Launch async_parallel_for
    auto for_handle = async_parallel_for(
        0,
        static_cast<int64_t>(N),
        1000,
        [&data1](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data1[i] = static_cast<int>(i + 1);
            }
        });

    // Launch another async_parallel_for
    auto for_handle2 = async_parallel_for(
        0,
        static_cast<int64_t>(N),
        1000,
        [&data2](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data2[i] = static_cast<int>(i * 2);
            }
        });

    // Wait for both
    for_handle.wait();
    for_handle2.wait();

    EXPECT_FALSE(for_handle.has_error());
    EXPECT_FALSE(for_handle2.has_error());

    // Now launch reductions on the results
    auto sum_handle1 = async_parallel_reduce(
        0,
        static_cast<int64_t>(N),
        1000,
        0LL,
        [&data1](int64_t begin, int64_t end, int64_t identity)
        {
            int64_t sum = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                sum += data1[i];
            }
            return sum;
        },
        [](int64_t a, int64_t b) { return a + b; });

    auto sum_handle2 = async_parallel_reduce(
        0,
        static_cast<int64_t>(N),
        1000,
        0LL,
        [&data2](int64_t begin, int64_t end, int64_t identity)
        {
            int64_t sum = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                sum += data2[i];
            }
            return sum;
        },
        [](int64_t a, int64_t b) { return a + b; });

    int64_t sum1 = sum_handle1.get();
    int64_t sum2 = sum_handle2.get();

    EXPECT_FALSE(sum_handle1.has_error());
    EXPECT_FALSE(sum_handle2.has_error());

    // Verify sums
    int64_t expected_sum1 = (N * (N + 1)) / 2;  // Sum of 1 to N
    int64_t expected_sum2 = 0;
    for (size_t i = 0; i < N; ++i)
    {
        expected_sum2 += i * 2;
    }

    EXPECT_EQ(sum1, expected_sum1);
    EXPECT_EQ(sum2, expected_sum2);
}

// Test 25: Batch of async operations
XSIGMATEST(AsyncParallel, batch_operations)
{
    const int    NUM_BATCHES = 20;
    const size_t BATCH_SIZE  = 1000;

    std::vector<std::vector<int>> batches(NUM_BATCHES);
    for (auto& batch : batches)
    {
        batch.resize(BATCH_SIZE, 0);
    }

    // Launch all batches
    std::vector<async_handle<void>> handles;
    handles.reserve(NUM_BATCHES);

    for (int b = 0; b < NUM_BATCHES; ++b)
    {
        handles.emplace_back(async_parallel_for(
            0,
            static_cast<int64_t>(BATCH_SIZE),
            100,
            [&batches, b](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    batches[b][i] = b * 1000 + static_cast<int>(i);
                }
            }));
    }

    // Wait for all
    int completed = 0;
    for (auto& handle : handles)
    {
        handle.wait();
        if (!handle.has_error())
        {
            ++completed;
        }
    }

    EXPECT_EQ(completed, NUM_BATCHES);

    // Verify results
    for (int b = 0; b < NUM_BATCHES; ++b)
    {
        for (size_t i = 0; i < BATCH_SIZE; ++i)
        {
            EXPECT_EQ(batches[b][i], b * 1000 + static_cast<int>(i));
        }
    }
}

}  // namespace xsigma
