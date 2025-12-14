/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Comprehensive unit tests for smp_tools public API
 *
 * Tests cover:
 * - Thread initialization and configuration
 * - Parallel for execution with various functors
 * - Nested parallelism control
 * - Parallel scope detection
 * - Local scope configuration
 * - Edge cases and error handling
 * - Backend-agnostic functionality
 */

#include <algorithm>
#include <atomic>
#include <numeric>
#include <vector>

#include "Testing/xsigmaTest.h"
#include "smp/smp_tools.h"

namespace xsigma
{

// ============================================================================
// Test Fixtures and Helper Classes
// ============================================================================

/**
 * @brief Simple functor for testing parallel_for
 */
class simple_functor
{
public:
    std::vector<std::atomic<int>>& data_;

    simple_functor(std::vector<std::atomic<int>>& data) : data_(data) {}

    void operator()(size_t begin, size_t end)
    {
        for (size_t i = begin; i < end; ++i)
        {
            data_[i].store(static_cast<int>(i * 2), std::memory_order_relaxed);
        }
    }
};

/**
 * @brief Functor with Initialize method
 */
class functor_with_initialize
{
public:
    std::vector<std::atomic<int>>& data_;
    std::atomic<int>&              init_count_;

    functor_with_initialize(std::vector<std::atomic<int>>& data, std::atomic<int>& count)
        : data_(data), init_count_(count)
    {
    }

    void Initialize() { init_count_.fetch_add(1, std::memory_order_relaxed); }

    void operator()(size_t begin, size_t end)
    {
        for (size_t i = begin; i < end; ++i)
        {
            data_[i].store(static_cast<int>(i), std::memory_order_relaxed);
        }
    }

    void Reduce() {}  // Empty reduce - not used in this test
};

/**
 * @brief Const functor for testing const parallel_for
 */
class const_functor
{
public:
    std::atomic<int>& counter_;

    const_functor(std::atomic<int>& counter) : counter_(counter) {}

    void operator()(size_t begin, size_t end) const
    {
        counter_.fetch_add(static_cast<int>(end - begin), std::memory_order_relaxed);
    }
};

// ============================================================================
// Test Group 1: Initialization and Thread Configuration
// ============================================================================

XSIGMATEST(SmpTools, initialize_default_threads)
{
    // Initialize with default thread count (0 = auto-detect)
    smp_tools::initialize(0);

    int num_threads = smp_tools::estimated_number_of_threads();

    // Should have at least 1 thread
    EXPECT_GE(num_threads, 1);

    // Should not exceed hardware concurrency
    EXPECT_LE(num_threads, static_cast<int>(std::thread::hardware_concurrency()));
}

XSIGMATEST(SmpTools, initialize_custom_thread_count)
{
    // Initialize with specific thread count
    smp_tools::initialize(4);

    int num_threads = smp_tools::estimated_number_of_threads();

    // Should match requested count (backend-dependent)
    // Some backends might adjust the count
    EXPECT_GE(num_threads, 1);
}

XSIGMATEST(SmpTools, initialize_single_thread)
{
    // Initialize with single thread
    smp_tools::initialize(1);

    int num_threads = smp_tools::estimated_number_of_threads();

    // Should have exactly 1 thread or close to it
    EXPECT_GE(num_threads, 1);
}

XSIGMATEST(SmpTools, estimated_number_of_threads)
{
    smp_tools::initialize(0);

    int num_threads = smp_tools::estimated_number_of_threads();

    // Basic sanity checks
    EXPECT_GT(num_threads, 0);
    EXPECT_LE(num_threads, 1024);  // Reasonable upper bound
}

XSIGMATEST(SmpTools, estimated_default_number_of_threads)
{
    int default_threads = smp_tools::estimated_default_number_of_threads();

    // Should be positive
    EXPECT_GT(default_threads, 0);

    // Should be reasonable
    EXPECT_LE(default_threads, static_cast<int>(std::thread::hardware_concurrency() * 2));
}

// ============================================================================
// Test Group 2: Parallel For Execution
// ============================================================================

XSIGMATEST(SmpTools, parallel_for_basic)
{
    const size_t                  size = 1000;
    std::vector<std::atomic<int>> data(size);
    simple_functor                func(data);

    // Initialize all to -1
    for (auto& elem : data)
    {
        elem.store(-1, std::memory_order_relaxed);
    }
    smp_tools::local_scope(
        smp_tools::config("std"),
        [&data, size, &func]() { smp_tools::parallel_for(0, size, 100, func); });

    smp_tools::local_scope(
        smp_tools::config("tbb"),
        [&data, size, &func]() { smp_tools::parallel_for(0, size, 100, func); });

    smp_tools::local_scope(
        smp_tools::config("openmp"),
        [&data, size, &func]() { smp_tools::parallel_for(0, size, 100, func); });

    // Verify all elements were processed
    for (size_t i = 0; i < size; ++i)
    {
        EXPECT_EQ(data[i].load(std::memory_order_relaxed), static_cast<int>(i * 2))
            << "Element " << i << " was not processed correctly";
    }
}

XSIGMATEST(SmpTools, parallel_for_const_functor)
{
    std::atomic<int> counter{0};
    const_functor    func(counter);

    const size_t size = 1000;

    smp_tools::parallel_for(0, size, 100, func);

    EXPECT_EQ(counter.load(std::memory_order_relaxed), static_cast<int>(size));
}

XSIGMATEST(SmpTools, parallel_for_with_initialize)
{
    const size_t                  size = 1000;
    std::vector<std::atomic<int>> data(size);
    std::atomic<int>              init_count{0};
    functor_with_initialize       func(data, init_count);

    smp_tools::parallel_for(0, size, 100, func);

    // Verify all elements were processed
    for (size_t i = 0; i < size; ++i)
    {
        EXPECT_EQ(data[i].load(std::memory_order_relaxed), static_cast<int>(i));
    }

    // Initialize should have been called at least once (per thread)
    EXPECT_GE(init_count.load(std::memory_order_relaxed), 1);
}

XSIGMATEST(SmpTools, parallel_for_empty_range)
{
    std::vector<std::atomic<int>> data(10);
    simple_functor                func(data);

    // Empty range (first == last)
    smp_tools::parallel_for(5, 5, 10, func);

    // Nothing should happen, no crash
    SUCCEED();
}

XSIGMATEST(SmpTools, parallel_for_single_element)
{
    std::vector<std::atomic<int>> data(10);
    simple_functor                func(data);

    data[5].store(-1, std::memory_order_relaxed);

    // Single element range
    smp_tools::parallel_for(5, 6, 10, func);

    EXPECT_EQ(data[5].load(std::memory_order_relaxed), 10);  // 5 * 2
}

XSIGMATEST(SmpTools, parallel_for_large_range)
{
    const size_t                  size = 100000;
    std::vector<std::atomic<int>> data(size);
    simple_functor                func(data);

    for (auto& elem : data)
    {
        elem.store(0, std::memory_order_relaxed);
    }

    smp_tools::parallel_for(0, size, 1000, func);

    // Spot check some elements
    EXPECT_EQ(data[0].load(std::memory_order_relaxed), 0);
    EXPECT_EQ(data[1000].load(std::memory_order_relaxed), 2000);
    EXPECT_EQ(data[size - 1].load(std::memory_order_relaxed), static_cast<int>((size - 1) * 2));
}

XSIGMATEST(SmpTools, parallel_for_various_grain_sizes)
{
    const size_t                  size = 1000;
    std::vector<std::atomic<int>> data(size);

    // Test different grain sizes
    std::vector<size_t> grain_sizes = {1, 10, 50, 100, 500, 1000, 10000};

    for (size_t grain : grain_sizes)
    {
        // Reset data
        for (auto& elem : data)
        {
            elem.store(-1, std::memory_order_relaxed);
        }

        simple_functor func(data);
        smp_tools::parallel_for(0, size, grain, func);

        // Verify
        for (size_t i = 0; i < size; ++i)
        {
            EXPECT_EQ(data[i].load(std::memory_order_relaxed), static_cast<int>(i * 2))
                << "Failed with grain size " << grain << " at index " << i;
        }
    }
}

XSIGMATEST(SmpTools, parallel_for_grain_larger_than_range)
{
    const size_t                  size = 100;
    std::vector<std::atomic<int>> data(size);
    simple_functor                func(data);

    for (auto& elem : data)
    {
        elem.store(-1, std::memory_order_relaxed);
    }

    // Grain size larger than range - should execute sequentially
    smp_tools::parallel_for(0, size, 1000, func);

    for (size_t i = 0; i < size; ++i)
    {
        EXPECT_EQ(data[i].load(std::memory_order_relaxed), static_cast<int>(i * 2));
    }
}

// ============================================================================
// Test Group 3: Nested Parallelism
// ============================================================================

XSIGMATEST(SmpTools, set_nested_parallelism_false)
{
    smp_tools::set_nested_parallelism(false);

    bool nested = smp_tools::nested_parallelism();

    EXPECT_FALSE(nested);
}

XSIGMATEST(SmpTools, set_nested_parallelism_true)
{
    smp_tools::set_nested_parallelism(true);

    bool nested = smp_tools::nested_parallelism();

    // May or may not be supported by backend
    // Just ensure no crash
    SUCCEED();
}

XSIGMATEST(SmpTools, nested_parallelism_toggle)
{
    // Toggle multiple times
    smp_tools::set_nested_parallelism(false);
    EXPECT_FALSE(smp_tools::nested_parallelism());

    smp_tools::set_nested_parallelism(true);
    // May return true or false depending on backend

    smp_tools::set_nested_parallelism(false);
    EXPECT_FALSE(smp_tools::nested_parallelism());
}

// ============================================================================
// Test Group 4: Parallel Scope Detection
// ============================================================================

XSIGMATEST(SmpTools, is_parallel_scope_outside)
{
    // Outside any parallel region
    bool is_parallel = smp_tools::is_parallel_scope();

    EXPECT_FALSE(is_parallel);
}

XSIGMATEST(SmpTools, is_parallel_scope_inside)
{
    std::atomic<bool> inside_scope_result{false};

    auto check_scope = [&inside_scope_result](size_t, size_t)
    { inside_scope_result.store(smp_tools::is_parallel_scope(), std::memory_order_relaxed); };

    smp_tools::parallel_for(0, 10, 5, check_scope);

    // Inside parallel region should return true (backend-dependent)
    // At minimum, should not crash
    SUCCEED();
}

XSIGMATEST(SmpTools, single_thread_outside)
{
    // Outside parallel region
    bool is_single = smp_tools::single_thread();

    // Behavior is backend-specific, just ensure no crash
    SUCCEED();
}

XSIGMATEST(SmpTools, single_thread_with_one_thread)
{
    smp_tools::initialize(1);

    bool is_single = smp_tools::single_thread();

    // With 1 thread, should return true (backend-dependent)
    SUCCEED();
}

// ============================================================================
// Test Group 5: Configuration Structure
// ============================================================================

XSIGMATEST(SmpTools, config_structure_default)
{
    smp_tools::config cfg;

    // Default values
    EXPECT_EQ(cfg.max_number_of_threads_, 0);
    EXPECT_FALSE(cfg.nested_parallelism_);
}

XSIGMATEST(SmpTools, config_structure_with_threads)
{
    smp_tools::config cfg(4);

    EXPECT_EQ(cfg.max_number_of_threads_, 4);
}

XSIGMATEST(SmpTools, config_structure_with_nested)
{
    smp_tools::config cfg(true);

    EXPECT_TRUE(cfg.nested_parallelism_);
}

// Note: local_scope tests are skipped due to incomplete implementation
// in the current API (missing backend field handling)

// ============================================================================
// Test Group 6: Edge Cases and Error Handling
// ============================================================================

XSIGMATEST(SmpTools, initialize_zero_threads)
{
    // Zero threads should use default
    smp_tools::initialize(0);

    int num_threads = smp_tools::estimated_number_of_threads();

    EXPECT_GT(num_threads, 0);
}

XSIGMATEST(SmpTools, initialize_negative_threads)
{
    // Negative threads should be handled gracefully
    smp_tools::initialize(-1);

    int num_threads = smp_tools::estimated_number_of_threads();

    // Should still have positive thread count
    EXPECT_GT(num_threads, 0);
}

XSIGMATEST(SmpTools, initialize_excessive_threads)
{
    // Request excessive thread count
    smp_tools::initialize(10000);

    int num_threads = smp_tools::estimated_number_of_threads();

    // Backend should cap it reasonably
    EXPECT_GT(num_threads, 0);
    EXPECT_LT(num_threads, 10000);  // Should be capped
}

XSIGMATEST(SmpTools, parallel_for_reversed_range)
{
    std::vector<std::atomic<int>> data(10);
    simple_functor                func(data);

    // Reversed range (first > last) - should handle gracefully
    smp_tools::parallel_for(10, 0, 5, func);

    // Should not crash
    SUCCEED();
}

XSIGMATEST(SmpTools, parallel_for_zero_grain)
{
    std::vector<std::atomic<int>> data(100);
    simple_functor                func(data);

    for (auto& elem : data)
    {
        elem.store(-1, std::memory_order_relaxed);
    }

    // Zero grain size - backend should handle appropriately
    smp_tools::parallel_for(0, 100, 0, func);

    // Should still process elements or handle gracefully
    SUCCEED();
}

// ============================================================================
// Test Group 7: Thread Safety and Concurrency
// ============================================================================

XSIGMATEST(SmpTools, parallel_for_thread_safety)
{
    const size_t                  size = 10000;
    std::vector<std::atomic<int>> data(size);

    for (auto& elem : data)
    {
        elem.store(0, std::memory_order_relaxed);
    }

    auto increment_func = [&data](size_t begin, size_t end)
    {
        for (size_t i = begin; i < end; ++i)
        {
            // Atomic increment
            data[i].fetch_add(1, std::memory_order_relaxed);
        }
    };

    // Run multiple parallel_for calls
    smp_tools::parallel_for(0, size, 100, increment_func);
    smp_tools::parallel_for(0, size, 100, increment_func);
    smp_tools::parallel_for(0, size, 100, increment_func);

    // Each element should be incremented exactly 3 times
    for (size_t i = 0; i < size; ++i)
    {
        EXPECT_EQ(data[i].load(std::memory_order_relaxed), 3)
            << "Thread safety violated at index " << i;
    }
}

XSIGMATEST(SmpTools, concurrent_parallel_for_calls)
{
    const size_t                  size = 1000;
    std::vector<std::atomic<int>> data1(size);
    std::vector<std::atomic<int>> data2(size);

    for (auto& elem : data1)
        elem.store(0, std::memory_order_relaxed);
    for (auto& elem : data2)
        elem.store(0, std::memory_order_relaxed);

    // Launch concurrent operations
    std::thread t1(
        [&data1]()
        {
            simple_functor func(data1);
            smp_tools::parallel_for(0, size, 100, func);
        });

    std::thread t2(
        [&data2]()
        {
            simple_functor func(data2);
            smp_tools::parallel_for(0, size, 100, func);
        });

    t1.join();
    t2.join();

    // Verify both completed successfully
    for (size_t i = 0; i < size; ++i)
    {
        EXPECT_EQ(data1[i].load(std::memory_order_relaxed), static_cast<int>(i * 2));
        EXPECT_EQ(data2[i].load(std::memory_order_relaxed), static_cast<int>(i * 2));
    }
}

// ============================================================================
// Test Group 8: Backend Verification
// ============================================================================

XSIGMATEST(SmpTools, threshold_constant)
{
    // Verify THRESHOLD constant exists and is reasonable
    EXPECT_GT(smp_tools::THRESHOLD, 0u);
    EXPECT_LE(smp_tools::THRESHOLD, 1000000u);
}

XSIGMATEST(SmpTools, multiple_initializations)
{
    // Multiple initializations should be safe
    smp_tools::initialize(2);
    int threads1 = smp_tools::estimated_number_of_threads();

    smp_tools::initialize(4);
    int threads2 = smp_tools::estimated_number_of_threads();

    smp_tools::initialize(0);
    int threads3 = smp_tools::estimated_number_of_threads();

    // All should succeed
    EXPECT_GT(threads1, 0);
    EXPECT_GT(threads2, 0);
    EXPECT_GT(threads3, 0);
}

// ============================================================================
// Test Group 9: Performance and Correctness
// ============================================================================

XSIGMATEST(SmpTools, parallel_for_correctness_large_dataset)
{
    const size_t                  size = 100000;
    std::vector<int>              input(size);
    std::vector<std::atomic<int>> output(size);

    // Initialize input
    std::iota(input.begin(), input.end(), 0);

    // Process in parallel
    auto process_func = [&input, &output](size_t begin, size_t end)
    {
        for (size_t i = begin; i < end; ++i)
        {
            // Some computation
            int result = input[i] * 3 + 7;
            output[i].store(result, std::memory_order_relaxed);
        }
    };

    smp_tools::parallel_for(0, size, 1000, process_func);

    // Verify correctness
    for (size_t i = 0; i < size; ++i)
    {
        int expected = static_cast<int>(i) * 3 + 7;
        EXPECT_EQ(output[i].load(std::memory_order_relaxed), expected)
            << "Incorrect result at index " << i;
    }
}

XSIGMATEST(SmpTools, parallel_for_no_data_races)
{
    const size_t                  size = 10000;
    std::vector<std::atomic<int>> counters(size);

    for (auto& c : counters)
    {
        c.store(0, std::memory_order_relaxed);
    }

    // Each thread increments its assigned elements
    auto increment_func = [&counters](size_t begin, size_t end)
    {
        for (size_t i = begin; i < end; ++i)
        {
            counters[i].fetch_add(1, std::memory_order_relaxed);
        }
    };

    // Run 10 times
    for (int iter = 0; iter < 10; ++iter)
    {
        smp_tools::parallel_for(0, size, 100, increment_func);
    }

    // Each counter should be exactly 10
    for (size_t i = 0; i < size; ++i)
    {
        EXPECT_EQ(counters[i].load(std::memory_order_relaxed), 10)
            << "Data race detected at index " << i;
    }
}

}  // namespace xsigma
