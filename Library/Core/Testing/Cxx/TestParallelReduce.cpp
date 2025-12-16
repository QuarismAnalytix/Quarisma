/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Comprehensive test suite for parallel_reduce functionality
 * Tests parallel reduction, edge cases, thread safety, and error handling
 */

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

#include "Testing/xsigmaTest.h"
#include "parallel/parallel.h"

namespace xsigma
{

// ============================================================================
// Test Group 1: Basic Functionality - Sum Reduction
// ============================================================================

// Test 1: Basic sum reduction
XSIGMATEST(ParallelReduce, Test)
{
    {
        int result = parallel_reduce(
            0,
            100,
            10,
            0,
            [](int64_t begin, int64_t end, int identity)
            {
                int sum = identity;
                for (int64_t i = begin; i < end; ++i)
                {
                    sum += static_cast<int>(i);
                }
                return sum;
            },
            [](int a, int b) { return a + b; });

        int expected = (99 * 100) / 2;  // Sum of 0..99
        EXPECT_EQ(result, expected);
    }

    // Test 2: Large range sum
    {
        int64_t result = parallel_reduce(
            0,
            10000,
            100,
            static_cast<int64_t>(0),
            [](int64_t begin, int64_t end, int64_t identity)
            {
                int64_t sum = identity;
                for (int64_t i = begin; i < end; ++i)
                {
                    sum += i;
                }
                return sum;
            },
            [](int64_t a, int64_t b) { return a + b; });

        int64_t expected = (9999LL * 10000LL) / 2;
        EXPECT_EQ(result, expected);
    }

    // ============================================================================
    // Test Group 2: Edge Cases
    // ============================================================================

    // Test 3: Empty range returns identity
    {
        int result = parallel_reduce(
            0,
            0,
            10,
            42,
            [](int64_t /*begin*/, int64_t /*end*/, int identity) { return identity; },
            [](int a, int b) { return a + b; });

        EXPECT_EQ(result, 42);
    }

    // Test 4: Single element
    {
        int result = parallel_reduce(
            0,
            1,
            10,
            0,
            [](int64_t begin, int64_t end, int identity)
            {
                int sum = identity;
                for (int64_t i = begin; i < end; ++i)
                {
                    sum += 10;
                }
                return sum;
            },
            [](int a, int b) { return a + b; });

        EXPECT_EQ(result, 10);
    }

    // Test 5: Range smaller than grain size
    {
        int result = parallel_reduce(
            0,
            5,
            100,
            0,
            [](int64_t begin, int64_t end, int identity)
            {
                int sum = identity;
                for (int64_t i = begin; i < end; ++i)
                {
                    sum += static_cast<int>(i);
                }
                return sum;
            },
            [](int a, int b) { return a + b; });

        int expected = 0 + 1 + 2 + 3 + 4;
        EXPECT_EQ(result, expected);
    }

    // ============================================================================
    // Test Group 3: Different Reduction Operations
    // ============================================================================

    // Test 6: Product reduction
    {
        int result = parallel_reduce(
            1,
            6,
            2,
            1,
            [](int64_t begin, int64_t end, int identity)
            {
                int product = identity;
                for (int64_t i = begin; i < end; ++i)
                {
                    product *= static_cast<int>(i);
                }
                return product;
            },
            [](int a, int b) { return a * b; });

        int expected = 1 * 2 * 3 * 4 * 5;  // 120
        EXPECT_EQ(result, expected);
    }

    // Test 7: Maximum reduction
    {
        std::vector<int> data(100);
        for (int i = 0; i < 100; ++i)
        {
            data[i] = (i * 37) % 100;  // Pseudo-random values
        }

        int result = parallel_reduce(
            0,
            100,
            10,
            std::numeric_limits<int>::min(),
            [&data](int64_t begin, int64_t end, int identity)
            {
                int max_val = identity;
                for (int64_t i = begin; i < end; ++i)
                {
                    max_val = std::max(max_val, data[i]);
                }
                return max_val;
            },
            [](int a, int b) { return std::max(a, b); });

        int expected = *std::max_element(data.begin(), data.end());
        EXPECT_EQ(result, expected);
    }

    // Test 8: Minimum reduction
    {
        std::vector<int> data(100);
        for (int i = 0; i < 100; ++i)
        {
            data[i] = (i * 37) % 100;
        }

        int result = parallel_reduce(
            0,
            100,
            10,
            std::numeric_limits<int>::max(),
            [&data](int64_t begin, int64_t end, int identity)
            {
                int min_val = identity;
                for (int64_t i = begin; i < end; ++i)
                {
                    min_val = std::min(min_val, data[i]);
                }
                return min_val;
            },
            [](int a, int b) { return std::min(a, b); });

        int expected = *std::min_element(data.begin(), data.end());
        EXPECT_EQ(result, expected);
    }

    // ============================================================================
    // Test Group 4: Different Data Types
    // ============================================================================

    // Test 9: Double precision sum
    {
        double result = parallel_reduce(
            0,
            100,
            10,
            0.0,
            [](int64_t begin, int64_t end, double identity)
            {
                double sum = identity;
                for (int64_t i = begin; i < end; ++i)
                {
                    sum += static_cast<double>(i) * 0.5;
                }
                return sum;
            },
            [](double a, double b) { return a + b; });

        double expected = (99.0 * 100.0 / 2.0) * 0.5;
        EXPECT_NEAR(result, expected, 1e-10);
    }

    // Test 10: Float precision product
    {
        float result = parallel_reduce(
            0,
            10,
            3,
            1.0f,
            [](int64_t begin, int64_t end, float identity)
            {
                float product = identity;
                for (int64_t i = begin; i < end; ++i)
                {
                    product *= (1.0f + static_cast<float>(i) * 0.1f);
                }
                return product;
            },
            [](float a, float b) { return a * b; });

        float expected = 1.0f;
        for (int i = 0; i < 10; ++i)
        {
            expected *= (1.0f + static_cast<float>(i) * 0.1f);
        }
        EXPECT_NEAR(result, expected, 1e-5f);
    }

    // ============================================================================
    // Test Group 5: Grain Size Variations
    // ============================================================================

    // Test 11: Very small grain size
    {
        int result = parallel_reduce(
            0,
            50,
            1,
            0,
            [](int64_t begin, int64_t end, int identity)
            {
                int sum = identity;
                for (int64_t i = begin; i < end; ++i)
                {
                    sum += static_cast<int>(i);
                }
                return sum;
            },
            [](int a, int b) { return a + b; });

        int expected = (49 * 50) / 2;
        EXPECT_EQ(result, expected);
    }

    // Test 12: Very large grain size
    {
        int result = parallel_reduce(
            0,
            100,
            1000,
            0,
            [](int64_t begin, int64_t end, int identity)
            {
                int sum = identity;
                for (int64_t i = begin; i < end; ++i)
                {
                    sum += static_cast<int>(i);
                }
                return sum;
            },
            [](int a, int b) { return a + b; });

        int expected = (99 * 100) / 2;
        EXPECT_EQ(result, expected);
    }
}
}  // namespace xsigma
