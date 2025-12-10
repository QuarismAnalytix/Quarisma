/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Comprehensive test suite for parallel_for functionality
 * Tests parallel iteration, edge cases, thread safety, and error handling
 */

#include <algorithm>
#include <atomic>
#include <numeric>
#include <vector>

#include "Testing/xsigmaTest.h"
#include "parallel/parallel.h"

namespace xsigma
{

// ============================================================================
// Test Group 1: Basic Functionality
// ============================================================================

// Test 1: Basic parallel_for with simple range
XSIGMATEST(ParallelFor, basic_range)
{
    std::vector<int> data(100, 0);

    parallel_for(
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

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(data[i], i * 2);
    }
}

// Test 2: Verify all elements are processed
XSIGMATEST(ParallelFor, all_elements_processed)
{
    // Use atomic<bool> to ensure memory visibility across threads
    std::vector<std::atomic<bool>> processed(1000);

    // Initialize all elements to false
    for (auto& elem : processed)
    {
        elem.store(false, std::memory_order_relaxed);
    }

    parallel_for(
        0,
        1000,
        50,
        [&processed](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                // Use release semantics to ensure all previous writes are visible
                processed[i].store(true, std::memory_order_release);
            }
        });

    // Verify all elements were processed
    for (size_t i = 0; i < processed.size(); ++i)
    {
        // Use acquire semantics to ensure we see all worker thread writes
        EXPECT_TRUE(processed[i].load(std::memory_order_acquire))
            << "Element " << i << " was not processed";
    }
}

// Test 3: Verify chunk distribution is reasonable
XSIGMATEST(ParallelFor, chunk_distribution)
{
    std::atomic<int> chunk_count{0};
    const int64_t    total_size = 100;
    const int64_t    grain_size = 10;

    parallel_for(
        0,
        total_size,
        grain_size,
        [&chunk_count](int64_t /*begin*/, int64_t /*end*/) { chunk_count++; });

    // The actual number of chunks depends on the number of threads and backend.
    // Always require at least one chunk. For openmp/native backends the number
    // of chunks is bounded by ceil(total_size/grain_size). TBB may oversubdivide
    // work beyond this bound due to dynamic splitting, so we relax the upper
    // bound in that case.
    EXPECT_GE(chunk_count.load(), 1);
#if !XSIGMA_HAS_TBB
    EXPECT_LE(chunk_count.load(), divup(total_size, grain_size));
#endif
}

// ============================================================================
// Test Group 2: Edge Cases
// ============================================================================

// Test 4: Empty range (begin == end)
XSIGMATEST(ParallelFor, empty_range)
{
    std::atomic<int> counter{0};

    parallel_for(0, 0, 10, [&counter](int64_t /*begin*/, int64_t /*end*/) { ++counter; });

    EXPECT_EQ(counter.load(), 0);
}

// Test 5: Single element range
XSIGMATEST(ParallelFor, single_element)
{
    std::vector<int> data(1, 0);

    parallel_for(
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

    EXPECT_EQ(data[0], 42);
}

// Test 6: Range smaller than grain size
XSIGMATEST(ParallelFor, range_smaller_than_grain)
{
    std::vector<int> data(5, 0);

    parallel_for(
        0,
        5,
        100,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] = static_cast<int>(i);
            }
        });

    for (int i = 0; i < 5; ++i)
    {
        EXPECT_EQ(data[i], i);
    }
}

// Test 7: Very small grain size (maximum parallelization)
XSIGMATEST(ParallelFor, small_grain_size)
{
    std::vector<int> data(50, 0);

    parallel_for(
        0,
        50,
        1,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] = static_cast<int>(i);
            }
        });

    for (int i = 0; i < 50; ++i)
    {
        EXPECT_EQ(data[i], i);
    }
}

// Test 8: Large grain size (minimal parallelization)
XSIGMATEST(ParallelFor, large_grain_size)
{
    std::vector<int> data(100, 0);

    parallel_for(
        0,
        100,
        1000,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] = static_cast<int>(i * 3);
            }
        });

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(data[i], i * 3);
    }
}

// ============================================================================
// Test Group 3: Thread Safety
// ============================================================================

// Test 9: Atomic operations
XSIGMATEST(ParallelFor, atomic_operations)
{
    std::atomic<int64_t> sum{0};

    parallel_for(
        0,
        1000,
        50,
        [&sum](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                sum.fetch_add(i, std::memory_order_relaxed);
            }
        });

    int64_t expected = (999 * 1000) / 2;  // Sum of 0..999
    EXPECT_EQ(sum.load(), expected);
}

// Test 10: No data races with independent writes
XSIGMATEST(ParallelFor, independent_writes)
{
    std::vector<int> data(1000);

    parallel_for(
        0,
        1000,
        25,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] = static_cast<int>(i * i);
            }
        });

    for (int i = 0; i < 1000; ++i)
    {
        EXPECT_EQ(data[i], i * i);
    }
}

// ============================================================================
// Test Group 4: Different Data Types
// ============================================================================

// Test 11: Double precision floating point
XSIGMATEST(ParallelFor, double_precision)
{
    std::vector<double> data(100, 0.0);

    parallel_for(
        0,
        100,
        10,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] = static_cast<double>(i) * 1.5;
            }
        });

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_DOUBLE_EQ(data[i], static_cast<double>(i) * 1.5);
    }
}

// Test 12: Large data structures
XSIGMATEST(ParallelFor, large_structures)
{
    struct LargeStruct
    {
        int64_t values[10];
    };

    std::vector<LargeStruct> data(100);

    parallel_for(
        0,
        100,
        10,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                for (int j = 0; j < 10; ++j)
                {
                    data[i].values[j] = i * 10 + j;
                }
            }
        });

    for (int i = 0; i < 100; ++i)
    {
        for (int j = 0; j < 10; ++j)
        {
            EXPECT_EQ(data[i].values[j], i * 10 + j);
        }
    }
}

}  // namespace xsigma
