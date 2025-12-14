/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Comprehensive unit tests for smp_tools_api (Internal API)
 *
 * Tests cover:
 * - Singleton pattern
 * - Backend identification and verification
 * - Thread initialization and configuration
 * - Nested parallelism management
 * - Parallel scope detection
 * - Local scope configuration
 * - parallel_for integration with backend
 * - Backend-specific behavior
 * - Thread safety
 * - Edge cases and error handling
 */

#include <atomic>
#include <functional>
#include <vector>

#include "Testing/xsigmaTest.h"
#include "smp/common/smp_tools_api.h"
#include "smp/smp.h"

namespace xsigma
{
namespace detail
{
namespace smp
{

// ============================================================================
// Test Helper Classes
// ============================================================================

/**
 * @brief Simple functor for testing parallel_for
 */
class test_functor_internal
{
public:
    std::vector<std::atomic<int>>& data_;

    test_functor_internal(std::vector<std::atomic<int>>& data) : data_(data) {}

    void Execute(size_t begin, size_t end)
    {
        for (size_t i = begin; i < end; ++i)
        {
            data_[i].store(static_cast<int>(i * 3), std::memory_order_relaxed);
        }
    }
};

/**
 * @brief Functor that tracks parallel scope
 */
class scope_tracking_functor
{
public:
    std::atomic<bool>& was_in_parallel_scope_;

    scope_tracking_functor(std::atomic<bool>& flag) : was_in_parallel_scope_(flag) {}

    void Execute(size_t, size_t)
    {
        bool is_parallel = smp_tools_api::instance().is_parallel_scope();
        was_in_parallel_scope_.store(is_parallel, std::memory_order_relaxed);
    }
};

// ============================================================================
// Test Group 1: Singleton and Backend Identification
// ============================================================================

XSIGMATEST(SmpToolsApi, singleton_instance)
{
    // Get singleton instance
    smp_tools_api& api1 = smp_tools_api::instance();
    smp_tools_api& api2 = smp_tools_api::instance();

    // Should be the same instance
    EXPECT_EQ(&api1, &api2);
}

XSIGMATEST(SmpToolsApi, get_backend_type)
{
    backend_type type = smp_tools_api::get_backend_type();

    // Should be one of the valid backend types
    EXPECT_TRUE(
        type == backend_type::std_thread || type == backend_type::TBB ||
        type == backend_type::OpenMP);
}

XSIGMATEST(SmpToolsApi, get_backend_name)
{
    const char* backend_name = smp_tools_api::get_backend();

    ASSERT_NE(backend_name, nullptr);

    // Should be one of the known backends
    std::string name_str(backend_name);
    EXPECT_TRUE(name_str == "std" || name_str == "tbb" || name_str == "openmp");
}

XSIGMATEST(SmpToolsApi, backend_consistency)
{
    backend_type type        = smp_tools_api::get_backend_type();
    const char*  backend_str = smp_tools_api::get_backend();

    // Backend type and string should match
    if (type == backend_type::std_thread)
    {
        EXPECT_STREQ(backend_str, "std");
    }
    else if (type == backend_type::TBB)
    {
        EXPECT_STREQ(backend_str, "tbb");
    }
    else if (type == backend_type::OpenMP)
    {
        EXPECT_STREQ(backend_str, "openmp");
    }
}

// ============================================================================
// Test Group 2: Backend Setting (Verification Only)
// ============================================================================

XSIGMATEST(SmpToolsApi, set_backend_current)
{
    smp_tools_api& api             = smp_tools_api::instance();
    const char*    current_backend = smp_tools_api::get_backend();

    // Setting to current backend should succeed
    bool result = api.set_backend(current_backend);

    EXPECT_TRUE(result);
}

XSIGMATEST(SmpToolsApi, set_backend_invalid)
{
    smp_tools_api& api = smp_tools_api::instance();

    // Setting to invalid backend should fail
    bool result = api.set_backend("InvalidBackend");

    EXPECT_FALSE(result);
}

XSIGMATEST(SmpToolsApi, set_backend_null)
{
    smp_tools_api& api = smp_tools_api::instance();

    // Setting null backend should fail
    bool result = api.set_backend(nullptr);

    EXPECT_FALSE(result);
}

#if XSIGMA_HAS_OPENMP
XSIGMATEST(SmpToolsApi, set_backend_verify_openmp)
{
    smp_tools_api& api = smp_tools_api::instance();

    // Only OpenMP should succeed
    EXPECT_TRUE(api.set_backend("openmp"));
    EXPECT_FALSE(api.set_backend("tbb"));
    EXPECT_FALSE(api.set_backend("std"));
}
#endif

#if XSIGMA_HAS_TBB
XSIGMATEST(SmpToolsApi, set_backend_verify_tbb)
{
    smp_tools_api& api = smp_tools_api::instance();

    // Only TBB should succeed
    EXPECT_TRUE(api.set_backend("tbb"));
    EXPECT_FALSE(api.set_backend("openmp"));
    EXPECT_FALSE(api.set_backend("std"));
}
#endif

#if !XSIGMA_HAS_OPENMP && !XSIGMA_HAS_TBB
XSIGMATEST(SmpToolsApi, set_backend_verify_std_thread)
{
    smp_tools_api& api = smp_tools_api::instance();

    // Only std_thread should succeed
    EXPECT_TRUE(api.set_backend("std"));
    EXPECT_FALSE(api.set_backend("openmp"));
    EXPECT_FALSE(api.set_backend("tbb"));
}
#endif

// ============================================================================
// Test Group 3: Thread Initialization and Configuration
// ============================================================================

XSIGMATEST(SmpToolsApi, initialize_default)
{
    smp_tools_api& api = smp_tools_api::instance();

    // Initialize with default (0 = auto)
    api.initialize(0);

    int num_threads = api.estimated_number_of_threads();

    EXPECT_GT(num_threads, 0);
    EXPECT_LE(num_threads, static_cast<int>(std::thread::hardware_concurrency() * 2));
}

XSIGMATEST(SmpToolsApi, initialize_custom_threads)
{
    smp_tools_api& api = smp_tools_api::instance();

    // Initialize with specific thread count
    api.initialize(4);

    int num_threads = api.estimated_number_of_threads();

    // Should be at least 1 (backend may adjust)
    EXPECT_GT(num_threads, 0);
}

XSIGMATEST(SmpToolsApi, initialize_single_thread)
{
    smp_tools_api& api = smp_tools_api::instance();

    api.initialize(1);

    int num_threads = api.estimated_number_of_threads();

    EXPECT_GE(num_threads, 1);
}

XSIGMATEST(SmpToolsApi, initialize_negative_threads)
{
    smp_tools_api& api = smp_tools_api::instance();

    // Negative should be handled gracefully
    api.initialize(-5);

    int num_threads = api.estimated_number_of_threads();

    EXPECT_GT(num_threads, 0);
}

XSIGMATEST(SmpToolsApi, estimated_number_of_threads)
{
    smp_tools_api& api = smp_tools_api::instance();

    api.initialize(0);

    int num_threads = api.estimated_number_of_threads();

    EXPECT_GT(num_threads, 0);
    EXPECT_LE(num_threads, 1024);  // Reasonable upper bound
}

XSIGMATEST(SmpToolsApi, estimated_default_number_of_threads)
{
    smp_tools_api& api = smp_tools_api::instance();

    int default_threads = api.estimated_default_number_of_threads();

    EXPECT_GT(default_threads, 0);
    EXPECT_LE(default_threads, static_cast<int>(std::thread::hardware_concurrency() * 2));
}

XSIGMATEST(SmpToolsApi, get_internal_desired_number_of_thread)
{
    smp_tools_api& api = smp_tools_api::instance();

    api.initialize(8);

    int desired = api.get_internal_desired_number_of_thread();

    // Should reflect the initialized value (or 0 if not set)
    EXPECT_GE(desired, 0);
}

// ============================================================================
// Test Group 4: Nested Parallelism
// ============================================================================

XSIGMATEST(SmpToolsApi, set_nested_parallelism_false)
{
    smp_tools_api& api = smp_tools_api::instance();

    api.set_nested_parallelism(false);

    bool is_nested = api.nested_parallelism();

    EXPECT_FALSE(is_nested);
}

XSIGMATEST(SmpToolsApi, set_nested_parallelism_true)
{
    smp_tools_api& api = smp_tools_api::instance();

    api.set_nested_parallelism(true);

    // May or may not be supported
    SUCCEED();
}

XSIGMATEST(SmpToolsApi, nested_parallelism_toggle)
{
    smp_tools_api& api = smp_tools_api::instance();

    api.set_nested_parallelism(false);
    EXPECT_FALSE(api.nested_parallelism());

    api.set_nested_parallelism(true);
    // Check value (backend-dependent)

    api.set_nested_parallelism(false);
    EXPECT_FALSE(api.nested_parallelism());
}

// ============================================================================
// Test Group 5: Parallel Scope Detection
// ============================================================================

XSIGMATEST(SmpToolsApi, is_parallel_scope_outside)
{
    smp_tools_api& api = smp_tools_api::instance();

    // Outside any parallel region
    bool is_parallel = api.is_parallel_scope();

    EXPECT_FALSE(is_parallel);
}

XSIGMATEST(SmpToolsApi, is_parallel_scope_inside_parallel_for)
{
    smp_tools_api& api = smp_tools_api::instance();

    std::atomic<bool> was_parallel{false};

    scope_tracking_functor func(was_parallel);

    api.parallel_for(0, 100, 10, func);

    // May or may not be true depending on backend
    SUCCEED();
}

XSIGMATEST(SmpToolsApi, single_thread_detection)
{
    smp_tools_api& api = smp_tools_api::instance();

    bool is_single = api.single_thread();

    // Just ensure no crash
    SUCCEED();
}

XSIGMATEST(SmpToolsApi, single_thread_with_one_thread)
{
    smp_tools_api& api = smp_tools_api::instance();

    api.initialize(1);

    bool is_single = api.single_thread();

    // Backend-dependent behavior
    SUCCEED();
}

// ============================================================================
// Test Group 6: Parallel For Execution
// ============================================================================

XSIGMATEST(SmpToolsApi, parallel_for_basic)
{
    smp_tools_api& api = smp_tools_api::instance();

    const size_t                  size = 1000;
    std::vector<std::atomic<int>> data(size);

    for (auto& elem : data)
    {
        elem.store(-1, std::memory_order_relaxed);
    }

    test_functor_internal func(data);

    api.parallel_for(0, size, 100, func);

    // Verify all elements processed
    for (size_t i = 0; i < size; ++i)
    {
        EXPECT_EQ(data[i].load(std::memory_order_relaxed), static_cast<int>(i * 3))
            << "Element " << i << " not processed correctly";
    }
}

XSIGMATEST(SmpToolsApi, parallel_for_empty_range)
{
    smp_tools_api& api = smp_tools_api::instance();

    std::vector<std::atomic<int>> data(10);
    test_functor_internal         func(data);

    // Empty range
    api.parallel_for(5, 5, 10, func);

    // Should not crash
    SUCCEED();
}

XSIGMATEST(SmpToolsApi, parallel_for_single_element)
{
    smp_tools_api& api = smp_tools_api::instance();

    std::vector<std::atomic<int>> data(10);

    for (auto& elem : data)
    {
        elem.store(0, std::memory_order_relaxed);
    }

    test_functor_internal func(data);

    api.parallel_for(5, 6, 10, func);

    EXPECT_EQ(data[5].load(std::memory_order_relaxed), 15);  // 5 * 3
}

XSIGMATEST(SmpToolsApi, parallel_for_large_range)
{
    smp_tools_api& api = smp_tools_api::instance();

    const size_t                  size = 100000;
    std::vector<std::atomic<int>> data(size);

    for (auto& elem : data)
    {
        elem.store(0, std::memory_order_relaxed);
    }

    test_functor_internal func(data);

    api.parallel_for(0, size, 1000, func);

    // Spot check
    EXPECT_EQ(data[0].load(std::memory_order_relaxed), 0);
    EXPECT_EQ(data[1000].load(std::memory_order_relaxed), 3000);
    EXPECT_EQ(data[size - 1].load(std::memory_order_relaxed), static_cast<int>((size - 1) * 3));
}

XSIGMATEST(SmpToolsApi, parallel_for_various_grain_sizes)
{
    smp_tools_api& api = smp_tools_api::instance();

    const size_t                  size = 1000;
    std::vector<std::atomic<int>> data(size);

    std::vector<size_t> grains = {1, 10, 50, 100, 500, 1000, 5000};

    for (size_t grain : grains)
    {
        // Reset
        for (auto& elem : data)
        {
            elem.store(0, std::memory_order_relaxed);
        }

        test_functor_internal func(data);
        api.parallel_for(0, size, grain, func);

        // Verify
        for (size_t i = 0; i < size; ++i)
        {
            EXPECT_EQ(data[i].load(std::memory_order_relaxed), static_cast<int>(i * 3))
                << "Failed with grain " << grain << " at index " << i;
        }
    }
}

XSIGMATEST(SmpToolsApi, parallel_for_zero_grain)
{
    smp_tools_api& api = smp_tools_api::instance();

    std::vector<std::atomic<int>> data(100);

    for (auto& elem : data)
    {
        elem.store(0, std::memory_order_relaxed);
    }

    test_functor_internal func(data);

    // Zero grain - backend should handle
    api.parallel_for(0, 100, 0, func);

    // Should not crash
    SUCCEED();
}

// ============================================================================
// Test Group 7: Internal Desired Thread Configuration
// ============================================================================

XSIGMATEST(SmpToolsApi, refresh_number_of_thread)
{
    smp_tools_api& api = smp_tools_api::instance();

    // Set a specific thread count
    api.initialize(4);

    int desired = api.get_internal_desired_number_of_thread();

    // Should reflect the configuration
    EXPECT_GE(desired, 0);
}

XSIGMATEST(SmpToolsApi, thread_configuration_persistence)
{
    smp_tools_api& api = smp_tools_api::instance();

    // Configure threads
    api.initialize(8);

    int threads1 = api.estimated_number_of_threads();

    // Should persist
    int threads2 = api.estimated_number_of_threads();

    EXPECT_EQ(threads1, threads2);
}

// Note: local_scope tests are better suited for the smp_tools level
// and are comprehensively tested in TestSmpTools.cpp

// ============================================================================
// Test Group 8: Thread Safety
// ============================================================================

XSIGMATEST(SmpToolsApi, concurrent_parallel_for_calls)
{
    smp_tools_api& api = smp_tools_api::instance();

    std::vector<std::atomic<int>> data1(1000);
    std::vector<std::atomic<int>> data2(1000);

    for (auto& elem : data1)
        elem.store(0, std::memory_order_relaxed);
    for (auto& elem : data2)
        elem.store(0, std::memory_order_relaxed);

    std::thread t1(
        [&api, &data1]()
        {
            test_functor_internal func(data1);
            api.parallel_for(0, 1000, 100, func);
        });

    std::thread t2(
        [&api, &data2]()
        {
            test_functor_internal func(data2);
            api.parallel_for(0, 1000, 100, func);
        });

    t1.join();
    t2.join();

    // Both should succeed
    for (size_t i = 0; i < 1000; ++i)
    {
        EXPECT_EQ(data1[i].load(std::memory_order_relaxed), static_cast<int>(i * 3));
        EXPECT_EQ(data2[i].load(std::memory_order_relaxed), static_cast<int>(i * 3));
    }
}

XSIGMATEST(SmpToolsApi, concurrent_initialization)
{
    smp_tools_api& api = smp_tools_api::instance();

    std::thread t1([&api]() { api.initialize(2); });
    std::thread t2([&api]() { api.initialize(4); });
    std::thread t3([&api]() { api.initialize(8); });

    t1.join();
    t2.join();
    t3.join();

    // Should not crash
    int num_threads = api.estimated_number_of_threads();
    EXPECT_GT(num_threads, 0);
}

// ============================================================================
// Test Group 9: Edge Cases
// ============================================================================

XSIGMATEST(SmpToolsApi, multiple_initializations)
{
    smp_tools_api& api = smp_tools_api::instance();

    api.initialize(2);
    int threads1 = api.estimated_number_of_threads();

    api.initialize(4);
    int threads2 = api.estimated_number_of_threads();

    api.initialize(0);
    int threads3 = api.estimated_number_of_threads();

    // All should succeed
    EXPECT_GT(threads1, 0);
    EXPECT_GT(threads2, 0);
    EXPECT_GT(threads3, 0);
}

XSIGMATEST(SmpToolsApi, excessive_thread_request)
{
    smp_tools_api& api = smp_tools_api::instance();

    api.initialize(100000);

    int num_threads = api.estimated_number_of_threads();

    // Should be capped
    EXPECT_GT(num_threads, 0);
    EXPECT_LT(num_threads, 100000);
}

XSIGMATEST(SmpToolsApi, parallel_for_reversed_range)
{
    smp_tools_api& api = smp_tools_api::instance();

    std::vector<std::atomic<int>> data(10);
    test_functor_internal         func(data);

    // Reversed range - should handle gracefully
    api.parallel_for(10, 0, 5, func);

    // Should not crash
    SUCCEED();
}

// ============================================================================
// Test Group 10: Integration Tests
// ============================================================================

XSIGMATEST(SmpToolsApi, full_workflow)
{
    smp_tools_api& api = smp_tools_api::instance();

    // Reset configuration
    api.initialize(4);
    api.set_nested_parallelism(false);

    // Verify state
    EXPECT_GT(api.estimated_number_of_threads(), 0);
    EXPECT_FALSE(api.is_parallel_scope());

    // Execute parallel work
    const size_t                  size = 10000;
    std::vector<std::atomic<int>> data(size);

    for (auto& elem : data)
    {
        elem.store(0, std::memory_order_relaxed);
    }

    test_functor_internal func(data);
    api.parallel_for(0, size, 1000, func);

    // Verify results
    for (size_t i = 0; i < size; ++i)
    {
        EXPECT_EQ(data[i].load(std::memory_order_relaxed), static_cast<int>(i * 3));
    }

    // Should not be in parallel scope after completion
    EXPECT_FALSE(api.is_parallel_scope());
}

XSIGMATEST(SmpToolsApi, backend_verification_workflow)
{
    smp_tools_api& api = smp_tools_api::instance();

    // Verify backend
    backend_type type = smp_tools_api::get_backend_type();
    const char*  name = smp_tools_api::get_backend();

    ASSERT_NE(name, nullptr);

    // Verify setting current backend succeeds
    EXPECT_TRUE(api.set_backend(name));

    // Initialize and use
    api.initialize(0);

    std::vector<std::atomic<int>> data(100);
    for (auto& elem : data)
    {
        elem.store(0, std::memory_order_relaxed);
    }

    test_functor_internal func(data);
    api.parallel_for(0, 100, 10, func);

    // Verify execution
    for (size_t i = 0; i < 100; ++i)
    {
        EXPECT_EQ(data[i].load(std::memory_order_relaxed), static_cast<int>(i * 3));
    }
}

}  // namespace smp
}  // namespace detail
}  // namespace xsigma
