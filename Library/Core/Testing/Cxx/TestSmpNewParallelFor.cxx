#include <algorithm>
#include <atomic>
#include <numeric>
#include <vector>

#include "Testing/xsigmaTest.h"
#include "smp_new/parallel/parallel_api.h"

namespace xsigma::smp_new::parallel
{

// Test 1: Basic parallel_for
XSIGMATEST(SmpNewParallelFor, basic)
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

// Test 2: Empty range
XSIGMATEST(SmpNewParallelFor, empty_range)
{
    std::atomic<int> counter{0};

    parallel_for(0, 0, 10, [&counter](int64_t begin, int64_t end) { ++counter; });

    EXPECT_EQ(counter.load(), 0);
}

// Test 3: Single element
XSIGMATEST(SmpNewParallelFor, single_element)
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

// Test 4: Small grain size
XSIGMATEST(SmpNewParallelFor, small_grain_size)
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

// Test 5: Large grain size
XSIGMATEST(SmpNewParallelFor, large_grain_size)
{
    std::vector<int> data(100, 0);

    parallel_for(
        0,
        100,
        100,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] = static_cast<int>(i);
            }
        });

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(data[i], i);
    }
}

// Test 6: Auto grain size (grain_size <= 0)
XSIGMATEST(SmpNewParallelFor, auto_grain_size)
{
    std::vector<int> data(100, 0);

    parallel_for(
        0,
        100,
        0,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] = static_cast<int>(i);
            }
        });

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(data[i], i);
    }
}

// Test 7: Large workload
XSIGMATEST(SmpNewParallelFor, large_workload)
{
    const int        size = 100000;
    std::vector<int> data(size, 0);

    parallel_for(
        0,
        size,
        1000,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] = static_cast<int>(i);
            }
        });

    for (int i = 0; i < size; ++i)
    {
        EXPECT_EQ(data[i], i);
    }
}

// Test 8: Atomic operations
XSIGMATEST(SmpNewParallelFor, atomic_operations)
{
    std::atomic<int> counter{0};

    parallel_for(
        0,
        1000,
        10,
        [&counter](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                counter.fetch_add(1, std::memory_order_relaxed);
            }
        });

    EXPECT_EQ(counter.load(), 1000);
}

// Test 9: Computation workload
XSIGMATEST(SmpNewParallelFor, computation_workload)
{
    const int           size = 1000;
    std::vector<double> data(size, 0.0);

    parallel_for(
        0,
        size,
        100,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                double x = static_cast<double>(i);
                data[i]  = x * x + 2.0 * x + 1.0;
            }
        });

    for (int i = 0; i < size; ++i)
    {
        double x        = static_cast<double>(i);
        double expected = x * x + 2.0 * x + 1.0;
        EXPECT_DOUBLE_EQ(data[i], expected);
    }
}

// Test 10: Thread safety with mutex
XSIGMATEST(SmpNewParallelFor, thread_safety_mutex)
{
    std::vector<int> data;
    std::mutex       data_mutex;

    parallel_for(
        0,
        100,
        10,
        [&data, &data_mutex](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                std::lock_guard<std::mutex> lock(data_mutex);
                data.push_back(static_cast<int>(i));
            }
        });

    EXPECT_EQ(data.size(), 100);
    std::sort(data.begin(), data.end());
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(data[i], i);
    }
}

// Test 11: Negative range (should handle gracefully)
XSIGMATEST(SmpNewParallelFor, negative_range)
{
    std::atomic<int> counter{0};

    parallel_for(10, 5, 10, [&counter](int64_t begin, int64_t end) { ++counter; });

    EXPECT_EQ(counter.load(), 0);
}

// Test 12: Very large grain size (larger than range)
XSIGMATEST(SmpNewParallelFor, very_large_grain_size)
{
    std::vector<int> data(10, 0);

    parallel_for(
        0,
        10,
        1000,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] = static_cast<int>(i);
            }
        });

    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(data[i], i);
    }
}

// Test 13: Multiple parallel_for calls
XSIGMATEST(SmpNewParallelFor, multiple_calls)
{
    std::vector<int> data1(100, 0);
    std::vector<int> data2(100, 0);

    parallel_for(
        0,
        100,
        10,
        [&data1](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data1[i] = static_cast<int>(i);
            }
        });

    parallel_for(
        0,
        100,
        10,
        [&data2](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data2[i] = static_cast<int>(i * 2);
            }
        });

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(data1[i], i);
        EXPECT_EQ(data2[i], i * 2);
    }
}

// Test 14: Different data types
XSIGMATEST(SmpNewParallelFor, different_data_types)
{
    std::vector<float>   floats(100, 0.0f);
    std::vector<int64_t> longs(100, 0);

    parallel_for(
        0,
        100,
        10,
        [&floats](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                floats[i] = static_cast<float>(i) * 1.5f;
            }
        });

    parallel_for(
        0,
        100,
        10,
        [&longs](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                longs[i] = i * 1000;
            }
        });

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_FLOAT_EQ(floats[i], static_cast<float>(i) * 1.5f);
        EXPECT_EQ(longs[i], i * 1000);
    }
}

XSIGMATEST(SmpNewParallelFor, restores_thread_state_serial)
{
    EXPECT_FALSE(in_parallel_region());
    EXPECT_EQ(get_thread_num(), 0);

    parallel_for(
        0,
        4,
        16,
        [](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                (void)i;
            }
        });

    EXPECT_FALSE(in_parallel_region());
    EXPECT_EQ(get_thread_num(), 0);
}

XSIGMATEST(SmpNewParallelFor, restores_thread_state_parallel)
{
    EXPECT_FALSE(in_parallel_region());

    parallel_for(
        0,
        256,
        1,
        [](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                (void)i;
            }
        });

    EXPECT_FALSE(in_parallel_region());
    EXPECT_EQ(get_thread_num(), 0);
}

}  // namespace xsigma::smp_new::parallel
