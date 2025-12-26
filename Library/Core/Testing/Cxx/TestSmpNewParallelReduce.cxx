#include <algorithm>
#include <numeric>
#include <vector>

#include "Testing/xsigmaTest.h"
#include "smp_new/parallel/parallel_api.h"

namespace xsigma::smp_new::parallel
{

// Test 1: Basic parallel_reduce - sum
XSIGMATEST(SmpNewParallelReduce, basic_sum)
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

// Test 2: Empty range
XSIGMATEST(SmpNewParallelReduce, empty_range)
{
    int result = parallel_reduce(
        0,
        0,
        10,
        42,
        [](int64_t begin, int64_t end, int identity) { return identity; },
        [](int a, int b) { return a + b; });

    EXPECT_EQ(result, 42);  // Should return identity
}

// Test 3: Single element
XSIGMATEST(SmpNewParallelReduce, single_element)
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

// Test 4: Product reduction
XSIGMATEST(SmpNewParallelReduce, product)
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

    EXPECT_EQ(result, 120);  // 1*2*3*4*5 = 120
}

// Test 5: Maximum reduction
XSIGMATEST(SmpNewParallelReduce, maximum)
{
    int result = parallel_reduce(
        0,
        100,
        10,
        std::numeric_limits<int>::min(),
        [](int64_t begin, int64_t end, int identity)
        {
            int max_val = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                max_val = std::max(max_val, static_cast<int>(i));
            }
            return max_val;
        },
        [](int a, int b) { return std::max(a, b); });

    EXPECT_EQ(result, 99);
}

// Test 6: Minimum reduction
XSIGMATEST(SmpNewParallelReduce, minimum)
{
    int result = parallel_reduce(
        0,
        100,
        10,
        std::numeric_limits<int>::max(),
        [](int64_t begin, int64_t end, int identity)
        {
            int min_val = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                min_val = std::min(min_val, static_cast<int>(i));
            }
            return min_val;
        },
        [](int a, int b) { return std::min(a, b); });

    EXPECT_EQ(result, 0);
}

// Test 7: Large workload
XSIGMATEST(SmpNewParallelReduce, large_workload)
{
    const int64_t size = 100000;

    int64_t result = parallel_reduce(
        0,
        size,
        1000,
        0LL,
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

    int64_t expected = (size - 1) * size / 2;
    EXPECT_EQ(result, expected);
}

// Test 8: Double precision reduction
XSIGMATEST(SmpNewParallelReduce, double_precision)
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

    double expected = (99 * 100) / 2 * 0.5;
    EXPECT_NEAR(result, expected, 1e-10);
}

// Test 9: Small grain size
XSIGMATEST(SmpNewParallelReduce, small_grain_size)
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

// Test 10: Large grain size
XSIGMATEST(SmpNewParallelReduce, large_grain_size)
{
    int result = parallel_reduce(
        0,
        100,
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

    int expected = (99 * 100) / 2;
    EXPECT_EQ(result, expected);
}

// Test 11: Auto grain size
XSIGMATEST(SmpNewParallelReduce, auto_grain_size)
{
    int result = parallel_reduce(
        0,
        100,
        0,
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

// Test 12: Count reduction
XSIGMATEST(SmpNewParallelReduce, count)
{
    int result = parallel_reduce(
        0,
        100,
        10,
        0,
        [](int64_t begin, int64_t end, int identity)
        {
            int count = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                if (i % 2 == 0)
                {
                    ++count;
                }
            }
            return count;
        },
        [](int a, int b) { return a + b; });

    EXPECT_EQ(result, 50);  // 50 even numbers in 0..99
}

// Test 13: Complex computation
XSIGMATEST(SmpNewParallelReduce, complex_computation)
{
    double result = parallel_reduce(
        0,
        1000,
        100,
        0.0,
        [](int64_t begin, int64_t end, double identity)
        {
            double sum = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                double x = static_cast<double>(i);
                sum += x * x + 2.0 * x + 1.0;
            }
            return sum;
        },
        [](double a, double b) { return a + b; });

    // Calculate expected value
    double expected = 0.0;
    for (int i = 0; i < 1000; ++i)
    {
        double x = static_cast<double>(i);
        expected += x * x + 2.0 * x + 1.0;
    }

    EXPECT_NEAR(result, expected, 1e-8);
}

// Test 14: Negative range
XSIGMATEST(SmpNewParallelReduce, negative_range)
{
    int result = parallel_reduce(
        10,
        5,
        10,
        42,
        [](int64_t begin, int64_t end, int identity) { return identity + 1; },
        [](int a, int b) { return a + b; });

    EXPECT_EQ(result, 42);  // Should return identity for invalid range
}

// Test 15: Very large grain size
XSIGMATEST(SmpNewParallelReduce, very_large_grain_size)
{
    int result = parallel_reduce(
        0,
        10,
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

    int expected = (9 * 10) / 2;
    EXPECT_EQ(result, expected);
}

// Test 16: Multiple reductions
XSIGMATEST(SmpNewParallelReduce, multiple_reductions)
{
    int sum = parallel_reduce(
        0,
        100,
        10,
        0,
        [](int64_t begin, int64_t end, int identity)
        {
            int s = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                s += static_cast<int>(i);
            }
            return s;
        },
        [](int a, int b) { return a + b; });

    int product = parallel_reduce(
        1,
        6,
        2,
        1,
        [](int64_t begin, int64_t end, int identity)
        {
            int p = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                p *= static_cast<int>(i);
            }
            return p;
        },
        [](int a, int b) { return a * b; });

    EXPECT_EQ(sum, (99 * 100) / 2);
    EXPECT_EQ(product, 120);
}

XSIGMATEST(SmpNewParallelReduce, restores_thread_state_serial)
{
    EXPECT_FALSE(in_parallel_region());
    EXPECT_EQ(get_thread_num(), 0);

    int result = parallel_reduce(
        0,
        4,
        16,
        0,
        [](int64_t begin, int64_t end, int identity)
        {
            int total = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                total += static_cast<int>(i);
            }
            return total;
        },
        [](int a, int b) { return a + b; });

    EXPECT_EQ(result, 6);
    EXPECT_FALSE(in_parallel_region());
    EXPECT_EQ(get_thread_num(), 0);
}

XSIGMATEST(SmpNewParallelReduce, restores_thread_state_parallel)
{
    EXPECT_FALSE(in_parallel_region());

    int result = parallel_reduce(
        0,
        256,
        1,
        0,
        [](int64_t begin, int64_t end, int identity)
        {
            int total = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                total += static_cast<int>(i);
            }
            return total;
        },
        [](int a, int b) { return a + b; });

    EXPECT_GT(result, 0);
    EXPECT_FALSE(in_parallel_region());
    EXPECT_EQ(get_thread_num(), 0);
}

}  // namespace xsigma::smp_new::parallel
