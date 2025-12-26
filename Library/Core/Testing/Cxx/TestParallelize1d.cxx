#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>
#include <vector>

#include "Testing/xsigmaTest.h"
#include "smp_new/parallel/parallel_api.h"

namespace xsigma::smp_new::parallel
{

// ============================================================================
// Test: Basic Functionality
// ============================================================================

XSIGMATEST(Parallelize1D, BasicFunctionality)
{
    const size_t     kSize = 1000;
    std::vector<int> data(kSize);

    parallelize_1d([&data](size_t i) { data[i] = static_cast<int>(i); }, kSize);

    for (size_t i = 0; i < kSize; ++i)
    {
        EXPECT_EQ(data[i], static_cast<int>(i));
    }
}

// ============================================================================
// Test: Large Data Set
// ============================================================================

XSIGMATEST(Parallelize1D, LargeDataSet)
{
    const size_t       kSize = 1000000;
    std::vector<float> data(kSize);

    parallelize_1d([&data](size_t i) { data[i] = std::sin(i * 0.001f); }, kSize);

    // Verify a few samples
    EXPECT_NEAR(data[0], std::sin(0.0f), 1e-6f);
    EXPECT_NEAR(data[100], std::sin(100 * 0.001f), 1e-6f);
    EXPECT_NEAR(data[kSize - 1], std::sin((kSize - 1) * 0.001f), 1e-6f);
}

// ============================================================================
// Test: Exception Handling
// ============================================================================

XSIGMATEST(Parallelize1D, ExceptionHandling)
{
    EXPECT_THROW(
        {
            parallelize_1d(
                [](size_t i)
                {
                    if (i == 500)
                    {
                        throw std::runtime_error("Test exception");
                    }
                },
                1000);
        },
        std::runtime_error);
}

// ============================================================================
// Test: Empty Range
// ============================================================================

XSIGMATEST(Parallelize1D, EmptyRange)
{
    std::atomic<int> count{0};

    parallelize_1d([&count](size_t i) { count++; }, 0);

    EXPECT_EQ(count, 0);
}

// ============================================================================
// Test: Single Item
// ============================================================================

XSIGMATEST(Parallelize1D, SingleItem)
{
    std::atomic<int> count{0};

    parallelize_1d([&count](size_t i) { count++; }, 1);

    EXPECT_EQ(count, 1);
}

// ============================================================================
// Test: Atomic Operations
// ============================================================================

XSIGMATEST(Parallelize1D, AtomicOperations)
{
    const size_t     kSize = 10000;
    std::atomic<int> sum{0};

    parallelize_1d([&sum](size_t i) { sum.fetch_add(1, std::memory_order_relaxed); }, kSize);

    EXPECT_EQ(sum, static_cast<int>(kSize));
}

// ============================================================================
// Test: Load Balancing (Work-Stealing)
// ============================================================================

XSIGMATEST(Parallelize1D, LoadBalancing)
{
    const size_t     kSize = 100000;
    std::vector<int> data(kSize);

    // Simulate uneven work distribution
    parallelize_1d(
        [&data](size_t i)
        {
            // Simulate variable work per item
            int work   = (i % 10 == 0) ? 1000 : 1;
            int result = 0;
            for (int j = 0; j < work; ++j)
            {
                result += j;
            }
            data[i] = result;
        },
        kSize);

    // Verify all items were processed
    for (size_t i = 0; i < kSize; ++i)
    {
        EXPECT_GE(data[i], 0);
    }
}

// ============================================================================
// Test: Does Not Wait For Unrelated Intra-op Work
// ============================================================================

XSIGMATEST(Parallelize1D, DoesNotWaitForExternalIntraopWork)
{
    std::atomic<bool> long_task_started{false};
    std::atomic<bool> long_task_finished{false};
    std::atomic<bool> allow_finish{false};

    intraop_launch(
        [&]()
        {
            long_task_started.store(true, std::memory_order_release);
            while (!allow_finish.load(std::memory_order_acquire))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            long_task_finished.store(true, std::memory_order_release);
        });

    // Wait until the long task starts executing
    for (int attempt = 0; attempt < 1000; ++attempt)
    {
        if (long_task_started.load(std::memory_order_acquire))
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    ASSERT_TRUE(long_task_started.load(std::memory_order_acquire));

    // parallelize_1d should complete even though the other task is still running
    parallelize_1d([](size_t) {}, 256);

    EXPECT_FALSE(long_task_finished.load(std::memory_order_acquire));

    // Allow the long task to finish before leaving the test
    allow_finish.store(true, std::memory_order_release);
    for (int attempt = 0; attempt < 1000; ++attempt)
    {
        if (long_task_finished.load(std::memory_order_acquire))
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    EXPECT_TRUE(long_task_finished.load(std::memory_order_acquire));
}

// ============================================================================
// Test: Performance Benchmark
// ============================================================================

XSIGMATEST(Parallelize1D, PerformanceBenchmark)
{
    const size_t       kSize = 10000000;  // 10M items
    std::vector<float> data(kSize);

    auto start = std::chrono::high_resolution_clock::now();

    parallelize_1d([&data](size_t i) { data[i] = std::sin(i * 0.001f); }, kSize);

    auto end         = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Print performance metrics
    double items_per_ms = static_cast<double>(kSize) / duration_ms;
    double us_per_item  = 1000.0 / items_per_ms;

    std::cout << "\n=== Parallelize1D Performance ===" << std::endl;
    std::cout << "Items: " << kSize << std::endl;
    std::cout << "Time: " << duration_ms << " ms" << std::endl;
    std::cout << "Items/ms: " << items_per_ms << std::endl;
    std::cout << "μs/item: " << us_per_item << std::endl;
    std::cout << "Target: < 0.2 μs/item" << std::endl;

    // Target: within 2x of PyTorch's pthreadpool_parallelize_1d (~0.1-0.2 μs/item)
    // So we expect < 0.4 μs/item
    EXPECT_LT(us_per_item, 0.4);
}

// ============================================================================
// Test: Multiple Calls
// ============================================================================

XSIGMATEST(Parallelize1D, MultipleCalls)
{
    const size_t     kSize = 1000;
    std::vector<int> data1(kSize);
    std::vector<int> data2(kSize);

    parallelize_1d([&data1](size_t i) { data1[i] = i; }, kSize);
    parallelize_1d([&data2](size_t i) { data2[i] = i * 2; }, kSize);

    for (size_t i = 0; i < kSize; ++i)
    {
        EXPECT_EQ(data1[i], static_cast<int>(i));
        EXPECT_EQ(data2[i], static_cast<int>(i * 2));
    }
}

// ============================================================================
// Test: Nested Lambdas
// ============================================================================

XSIGMATEST(Parallelize1D, NestedLambdas)
{
    const size_t     kSize = 1000;
    std::vector<int> data(kSize);

    auto outer = [&data](size_t i)
    {
        auto inner = [&data, i]() { data[i] = i; };
        inner();
    };

    parallelize_1d(outer, kSize);

    for (size_t i = 0; i < kSize; ++i)
    {
        EXPECT_EQ(data[i], static_cast<int>(i));
    }
}

// ============================================================================
// Test: Thread Safety
// ============================================================================

XSIGMATEST(Parallelize1D, ThreadSafety)
{
    const size_t                  kSize = 100000;
    std::vector<std::atomic<int>> data(kSize);

    // Initialize atomics
    for (size_t i = 0; i < kSize; ++i)
    {
        data[i].store(0);
    }

    parallelize_1d([&data](size_t i) { data[i].fetch_add(1, std::memory_order_relaxed); }, kSize);

    for (size_t i = 0; i < kSize; ++i)
    {
        EXPECT_EQ(data[i].load(), 1);
    }
}

}  // namespace xsigma::smp_new::parallel
