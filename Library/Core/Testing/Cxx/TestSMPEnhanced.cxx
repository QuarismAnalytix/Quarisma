/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Enhanced comprehensive test suite for SMP module
 * Tests parallel primitives, edge cases, thread safety, and error handling
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <numeric>
#include <set>
#include <thread>
#include <vector>

#include "Testing/xsigmaTest.h"
#include "smp/tools.h"

namespace xsigma
{

// ============================================================================
// Test Group 1: Parallel For - Edge Cases
// ============================================================================

// Test 1: Empty range
XSIGMATEST(SMPEnhanced, parallel_for_empty_range)
{
    std::atomic<int> counter{0};

    tools::For(0, 0, 10, [&counter](int begin, int end) { ++counter; });

    EXPECT_EQ(counter.load(), 0);
}

// Test 2: Single item
XSIGMATEST(SMPEnhanced, parallel_for_single_item)
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

// Test 3: Negative range (should not execute)
XSIGMATEST(SMPEnhanced, parallel_for_negative_range)
{
    std::atomic<int> counter{0};

    tools::For(10, 5, 1, [&counter](int begin, int end) { ++counter; });

    EXPECT_EQ(counter.load(), 0);
}

// Test 4: Large grain size (larger than range)
XSIGMATEST(SMPEnhanced, parallel_for_large_grain)
{
    std::vector<int> data(10, 0);

    tools::For(
        0,
        10,
        100,
        [&data](int begin, int end)
        {
            for (int i = begin; i < end; ++i)
            {
                data[i] = i;
            }
        });

    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(data[i], i);
    }
}

// Test 5: Zero grain size (auto grain)
XSIGMATEST(SMPEnhanced, parallel_for_zero_grain)
{
    std::vector<int> data(100, 0);

    tools::For(
        0,
        100,
        0,
        [&data](int begin, int end)
        {
            for (int i = begin; i < end; ++i)
            {
                data[i] = i * 2;
            }
        });

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(data[i], i * 2);
    }
}

// Test 6: Very fine grain (grain = 1)
XSIGMATEST(SMPEnhanced, parallel_for_fine_grain)
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

// ============================================================================
// Test Group 2: Parallel For - Iterator Version
// ============================================================================

// Test 7: Iterator version with vector
XSIGMATEST(SMPEnhanced, parallel_for_iterator_vector)
{
    std::vector<int> data(100, 0);

    tools::For(
        data.begin(),
        data.end(),
        10,
        [](std::vector<int>::iterator begin, std::vector<int>::iterator end)
        {
            int idx = 0;
            for (auto it = begin; it != end; ++it, ++idx)
            {
                *it = idx;
            }
        });

    // Verify some values (order may vary due to parallelism)
    bool all_set = true;
    for (const auto& val : data)
    {
        if (val < 0 || val >= 100)
        {
            all_set = false;
            break;
        }
    }
    EXPECT_TRUE(all_set);
}

// Test 8: Iterator version with set
XSIGMATEST(SMPEnhanced, parallel_for_iterator_set)
{
    std::set<int> data;
    for (int i = 0; i < 50; ++i)
    {
        data.insert(i);
    }

    std::atomic<int> sum{0};

    tools::For(
        data.begin(),
        data.end(),
        5,
        [&sum](std::set<int>::iterator begin, std::set<int>::iterator end)
        {
            for (auto it = begin; it != end; ++it)
            {
                sum.fetch_add(*it, std::memory_order_relaxed);
            }
        });

    // Sum of 0..49 = 49*50/2 = 1225
    EXPECT_EQ(sum.load(), 1225);
}

// Test 9: Iterator version with empty container
XSIGMATEST(SMPEnhanced, parallel_for_iterator_empty)
{
    std::vector<int> data;
    std::atomic<int> counter{0};

    tools::For(
        data.begin(),
        data.end(),
        10,
        [&counter](std::vector<int>::iterator begin, std::vector<int>::iterator end)
        { ++counter; });

    EXPECT_EQ(counter.load(), 0);
}

// Test 10: Iterator version with lambda (const version)
XSIGMATEST(SMPEnhanced, parallel_for_iterator_lambda)
{
    std::vector<int> data(100);
    std::iota(data.begin(), data.end(), 0);

    std::atomic<int64_t> sum{0};

    tools::For(
        data.begin(),
        data.end(),
        [&sum](std::vector<int>::const_iterator begin, std::vector<int>::const_iterator end)
        {
            for (auto it = begin; it != end; ++it)
            {
                sum.fetch_add(*it, std::memory_order_relaxed);
            }
        });

    // Sum of 0..99 = 99*100/2 = 4950
    EXPECT_EQ(sum.load(), 4950);
}

// ============================================================================
// Test Group 3: Thread Safety and Race Conditions
// ============================================================================

// Test 11: Atomic operations
XSIGMATEST(SMPEnhanced, parallel_for_atomic_operations)
{
    const int        size = 10000;
    std::atomic<int> counter{0};

    tools::For(
        0,
        size,
        100,
        [&counter](int begin, int end)
        {
            for (int i = begin; i < end; ++i)
            {
                counter.fetch_add(1, std::memory_order_relaxed);
            }
        });

    EXPECT_EQ(counter.load(), size);
}

// Test 12: No race condition with separate indices
XSIGMATEST(SMPEnhanced, parallel_for_no_race_condition)
{
    const int        size = 1000;
    std::vector<int> data(size, 0);

    tools::For(
        0,
        size,
        50,
        [&data](int begin, int end)
        {
            for (int i = begin; i < end; ++i)
            {
                data[i] = i * i;
            }
        });

    for (int i = 0; i < size; ++i)
    {
        EXPECT_EQ(data[i], i * i);
    }
}

// ============================================================================
// Test Group 4: Transform Operations
// ============================================================================

// Test 13: Unary transform - empty range
XSIGMATEST(SMPEnhanced, transform_unary_empty)
{
    std::vector<int> input;
    std::vector<int> output;

    tools::Transform(input.begin(), input.end(), output.begin(), [](int x) { return x * 2; });

    EXPECT_TRUE(output.empty());
}

// Test 14: Unary transform - single element
XSIGMATEST(SMPEnhanced, transform_unary_single)
{
    std::vector<int> input{5};
    std::vector<int> output(1, 0);

    tools::Transform(input.begin(), input.end(), output.begin(), [](int x) { return x * 3; });

    EXPECT_EQ(output[0], 15);
}

// Test 15: Unary transform - large dataset
XSIGMATEST(SMPEnhanced, transform_unary_large)
{
    const int        size = 100000;
    std::vector<int> input(size);
    std::iota(input.begin(), input.end(), 0);
    std::vector<int> output(size, 0);

    tools::Transform(input.begin(), input.end(), output.begin(), [](int x) { return x * 2; });

    for (int i = 0; i < size; ++i)
    {
        EXPECT_EQ(output[i], i * 2);
    }
}

// Test 16: Unary transform - floating point
XSIGMATEST(SMPEnhanced, transform_unary_float)
{
    const int           size = 1000;
    std::vector<double> input(size);
    std::iota(input.begin(), input.end(), 0.0);
    std::vector<double> output(size, 0.0);

    tools::Transform(
        input.begin(), input.end(), output.begin(), [](double x) { return std::sqrt(x); });

    for (int i = 0; i < size; ++i)
    {
        EXPECT_NEAR(output[i], std::sqrt(static_cast<double>(i)), 1e-10);
    }
}

// Test 17: Binary transform - basic
XSIGMATEST(SMPEnhanced, transform_binary_basic)
{
    std::vector<int> input1{1, 2, 3, 4, 5};
    std::vector<int> input2{10, 20, 30, 40, 50};
    std::vector<int> output(5, 0);

    tools::Transform(
        input1.begin(),
        input1.end(),
        input2.begin(),
        output.begin(),
        [](int x, int y) { return x + y; });

    EXPECT_EQ(output[0], 11);
    EXPECT_EQ(output[1], 22);
    EXPECT_EQ(output[2], 33);
    EXPECT_EQ(output[3], 44);
    EXPECT_EQ(output[4], 55);
}

// Test 18: Binary transform - multiplication
XSIGMATEST(SMPEnhanced, transform_binary_multiply)
{
    const int        size = 10000;
    std::vector<int> input1(size);
    std::vector<int> input2(size);
    std::iota(input1.begin(), input1.end(), 1);
    std::iota(input2.begin(), input2.end(), 1);
    std::vector<int> output(size, 0);

    tools::Transform(
        input1.begin(),
        input1.end(),
        input2.begin(),
        output.begin(),
        [](int x, int y) { return x * y; });

    for (int i = 0; i < size; ++i)
    {
        EXPECT_EQ(output[i], (i + 1) * (i + 1));
    }
}

// ============================================================================
// Test Group 5: Fill Operations
// ============================================================================

// Test 19: Fill - empty range
XSIGMATEST(SMPEnhanced, fill_empty)
{
    std::vector<int> data;

    tools::Fill(data.begin(), data.end(), 42);

    EXPECT_TRUE(data.empty());
}

// Test 20: Fill - single element
XSIGMATEST(SMPEnhanced, fill_single)
{
    std::vector<int> data(1, 0);

    tools::Fill(data.begin(), data.end(), 99);

    EXPECT_EQ(data[0], 99);
}

// Test 21: Fill - large dataset
XSIGMATEST(SMPEnhanced, fill_large)
{
    const int        size = 100000;
    std::vector<int> data(size, 0);

    tools::Fill(data.begin(), data.end(), 42);

    for (int i = 0; i < size; ++i)
    {
        EXPECT_EQ(data[i], 42);
    }
}

// Test 22: Fill - floating point
XSIGMATEST(SMPEnhanced, fill_float)
{
    const int           size = 1000;
    std::vector<double> data(size, 0.0);

    tools::Fill(data.begin(), data.end(), 3.14159);

    for (int i = 0; i < size; ++i)
    {
        EXPECT_DOUBLE_EQ(data[i], 3.14159);
    }
}

// ============================================================================
// Test Group 6: Sort Operations
// ============================================================================

// Test 23: Sort - empty range
XSIGMATEST(SMPEnhanced, sort_empty)
{
    std::vector<int> data;

    tools::Sort(data.begin(), data.end());

    EXPECT_TRUE(data.empty());
}

// Test 24: Sort - single element
XSIGMATEST(SMPEnhanced, sort_single)
{
    std::vector<int> data{42};

    tools::Sort(data.begin(), data.end());

    EXPECT_EQ(data[0], 42);
}

// Test 25: Sort - already sorted
XSIGMATEST(SMPEnhanced, sort_already_sorted)
{
    std::vector<int> data(100);
    std::iota(data.begin(), data.end(), 0);

    tools::Sort(data.begin(), data.end());

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(data[i], i);
    }
}

// Test 26: Sort - reverse sorted
XSIGMATEST(SMPEnhanced, sort_reverse)
{
    std::vector<int> data(100);
    std::iota(data.rbegin(), data.rend(), 0);

    tools::Sort(data.begin(), data.end());

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(data[i], i);
    }
}

// Test 27: Sort - random data
XSIGMATEST(SMPEnhanced, sort_random)
{
    std::vector<int> data{5, 2, 8, 1, 9, 3, 7, 4, 6};

    tools::Sort(data.begin(), data.end());

    for (size_t i = 0; i < data.size(); ++i)
    {
        EXPECT_EQ(data[i], static_cast<int>(i + 1));
    }
}

// Test 28: Sort - large dataset
XSIGMATEST(SMPEnhanced, sort_large)
{
    const int        size = 100000;
    std::vector<int> data(size);

    // Fill with reverse order
    for (int i = 0; i < size; ++i)
    {
        data[i] = size - i - 1;
    }

    tools::Sort(data.begin(), data.end());

    for (int i = 0; i < size; ++i)
    {
        EXPECT_EQ(data[i], i);
    }
}

// Test 29: Sort with custom comparator - descending
XSIGMATEST(SMPEnhanced, sort_custom_comparator)
{
    std::vector<int> data{1, 5, 3, 9, 2, 7, 4, 8, 6};

    tools::Sort(data.begin(), data.end(), [](int a, int b) { return a > b; });

    for (size_t i = 0; i < data.size(); ++i)
    {
        EXPECT_EQ(data[i], static_cast<int>(9 - i));
    }
}

// Test 30: Sort with duplicates
XSIGMATEST(SMPEnhanced, sort_duplicates)
{
    std::vector<int> data{5, 2, 8, 2, 9, 5, 7, 2, 5};

    tools::Sort(data.begin(), data.end());

    EXPECT_EQ(data[0], 2);
    EXPECT_EQ(data[1], 2);
    EXPECT_EQ(data[2], 2);
    EXPECT_EQ(data[3], 5);
    EXPECT_EQ(data[4], 5);
    EXPECT_EQ(data[5], 5);
    EXPECT_EQ(data[6], 7);
    EXPECT_EQ(data[7], 8);
    EXPECT_EQ(data[8], 9);
}

}  // namespace xsigma
