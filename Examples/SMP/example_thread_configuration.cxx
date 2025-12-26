/**
 * @file example_thread_configuration.cxx
 * @brief Demonstrates thread pool configuration and backend selection in XSigma SMP.
 *
 * This example shows:
 * - Querying available backends
 * - Configuring thread pool size
 * - Nested parallelism control
 * - Single-threaded mode for debugging
 * - Performance impact of different configurations
 */

#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

#include "smp/tools.h"
#include "smp_new/parallel/parallel_api.h"

namespace xsigma::examples::smp
{

/**
 * @brief Example 1: Query available backends and current configuration.
 *
 * Demonstrates how to query the SMP backend and thread configuration.
 */
void example_query_configuration()
{
    std::cout << "\n=== Example 1: Query Configuration ===" << std::endl;

    // Get current backend
    const char* backend = xsigma::smp::tools::GetBackend();
    std::cout << "Current backend: " << (backend ? backend : "default") << std::endl;

    // Get estimated number of threads
    int num_threads = xsigma::smp::tools::GetEstimatedNumberOfThreads();
    std::cout << "Estimated number of threads: " << num_threads << std::endl;

    // Get default number of threads
    int default_threads = xsigma::smp::tools::GetEstimatedDefaultNumberOfThreads();
    std::cout << "Default number of threads: " << default_threads << std::endl;

    // Check nested parallelism support
    bool nested = xsigma::smp::tools::GetNestedParallelism();
    std::cout << "Nested parallelism: " << (nested ? "enabled" : "disabled") << std::endl;

    // Check if in parallel scope
    bool in_parallel = xsigma::smp::tools::IsParallelScope();
    std::cout << "Currently in parallel scope: " << (in_parallel ? "yes" : "no") << std::endl;

    // Check single-threaded mode
    bool single_thread = xsigma::smp::tools::GetSingleThread();
    std::cout << "Single-threaded mode: " << (single_thread ? "enabled" : "disabled") << std::endl;
}

/**
 * @brief Example 2: Initialize thread pool with specific number of threads.
 *
 * Demonstrates explicit thread pool initialization.
 */
void example_initialize_thread_pool()
{
    std::cout << "\n=== Example 2: Initialize Thread Pool ===" << std::endl;

    // Initialize with specific number of threads
    int num_threads = 4;
    xsigma::smp::tools::Initialize(num_threads);
    std::cout << "Initialized thread pool with " << num_threads << " threads" << std::endl;

    // Verify initialization
    int actual_threads = xsigma::smp::tools::GetEstimatedNumberOfThreads();
    std::cout << "Actual number of threads: " << actual_threads << std::endl;
}

/**
 * @brief Example 3: Performance comparison with different thread counts.
 *
 * Demonstrates how thread count affects performance.
 */
void example_thread_count_performance()
{
    std::cout << "\n=== Example 3: Thread Count Performance ===" << std::endl;

    const size_t       kDataSize = 100000000;
    std::vector<float> data(kDataSize);

    // Test with different thread counts
    std::vector<int> thread_counts = {1, 2, 4, 8};

    for (int num_threads : thread_counts)
    {
        // Initialize with specific thread count
        xsigma::smp::tools::Initialize(num_threads);

        auto start = std::chrono::high_resolution_clock::now();

        // Perform parallel computation
        xsigma::smp_new::parallel::parallel_for(
            0,
            static_cast<int64_t>(kDataSize),
            1000000,
            [&data](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    data[i] = std::sin(i * 0.0001f);
                }
            });

        auto end      = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "Threads: " << num_threads << " -> Time: " << duration.count() << " ms"
                  << std::endl;
    }
}

/**
 * @brief Example 4: Nested parallelism control.
 *
 * Demonstrates enabling/disabling nested parallelism and its impact.
 */
void example_nested_parallelism_control()
{
    std::cout << "\n=== Example 4: Nested Parallelism Control ===" << std::endl;

    const int64_t                   kOuterSize = 100;
    const int64_t                   kInnerSize = 10000;
    std::vector<std::vector<float>> data(kOuterSize, std::vector<float>(kInnerSize));

    // Test with nested parallelism enabled
    std::cout << "\nWith nested parallelism enabled:" << std::endl;
    xsigma::smp::tools::SetNestedParallelism(true);
    std::cout << "Nested parallelism: "
              << (xsigma::smp::tools::GetNestedParallelism() ? "ON" : "OFF") << std::endl;

    auto start1 = std::chrono::high_resolution_clock::now();

    xsigma::smp_new::parallel::parallel_for(
        0,
        kOuterSize,
        10,
        [&data, kInnerSize](int64_t outer_begin, int64_t outer_end)
        {
            for (int64_t i = outer_begin; i < outer_end; ++i)
            {
                xsigma::smp_new::parallel::parallel_for(
                    0,
                    kInnerSize,
                    1000,
                    [&data, i](int64_t inner_begin, int64_t inner_end)
                    {
                        for (int64_t j = inner_begin; j < inner_end; ++j)
                        {
                            data[i][j] = std::sin(i * j * 0.0001f);
                        }
                    });
            }
        });

    auto end1      = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);

    std::cout << "Time with nested parallelism: " << duration1.count() << " ms" << std::endl;

    // Test with nested parallelism disabled
    std::cout << "\nWith nested parallelism disabled:" << std::endl;
    xsigma::smp::tools::SetNestedParallelism(false);
    std::cout << "Nested parallelism: "
              << (xsigma::smp::tools::GetNestedParallelism() ? "ON" : "OFF") << std::endl;

    auto start2 = std::chrono::high_resolution_clock::now();

    xsigma::smp_new::parallel::parallel_for(
        0,
        kOuterSize,
        10,
        [&data, kInnerSize](int64_t outer_begin, int64_t outer_end)
        {
            for (int64_t i = outer_begin; i < outer_end; ++i)
            {
                xsigma::smp_new::parallel::parallel_for(
                    0,
                    kInnerSize,
                    1000,
                    [&data, i](int64_t inner_begin, int64_t inner_end)
                    {
                        for (int64_t j = inner_begin; j < inner_end; ++j)
                        {
                            data[i][j] = std::sin(i * j * 0.0001f);
                        }
                    });
            }
        });

    auto end2      = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);

    std::cout << "Time without nested parallelism: " << duration2.count() << " ms" << std::endl;
}

/**
 * @brief Example 5: Single-threaded mode for debugging.
 *
 * Demonstrates using single-threaded mode for debugging parallel code.
 */
void example_single_threaded_mode()
{
    std::cout << "\n=== Example 5: Single-Threaded Mode ===" << std::endl;

    const size_t       kDataSize = 1000000;
    std::vector<float> data(kDataSize);

    // Enable single-threaded mode
    std::cout << "Enabling single-threaded mode for debugging..." << std::endl;
    xsigma::smp::tools::SetNestedParallelism(false);
    xsigma::smp::tools::Initialize(1);

    auto start = std::chrono::high_resolution_clock::now();

    // This will execute sequentially even though we use parallel APIs
    xsigma::smp_new::parallel::parallel_for(
        0,
        static_cast<int64_t>(kDataSize),
        10000,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] = std::sin(i * 0.001f);
            }
        });

    auto end      = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Single-threaded execution time: " << duration.count() << " ms" << std::endl;
    std::cout << "Note: This mode is useful for debugging race conditions" << std::endl;
}

/**
 * @brief Example 6: Backend selection and configuration.
 *
 * Demonstrates selecting different SMP backends (if available).
 */
void example_backend_selection()
{
    std::cout << "\n=== Example 6: Backend Selection ===" << std::endl;

    // Query current backend
    const char* current_backend = xsigma::smp::tools::GetBackend();
    std::cout << "Current backend: " << (current_backend ? current_backend : "default")
              << std::endl;

    // Try to set a specific backend (if supported)
    // Note: This is backend-dependent and may not be supported on all systems
    bool success = xsigma::smp::tools::SetBackend("STDThread");
    if (success)
    {
        std::cout << "Successfully set backend to STDThread" << std::endl;
        const char* new_backend = xsigma::smp::tools::GetBackend();
        std::cout << "New backend: " << (new_backend ? new_backend : "default") << std::endl;
    }
    else
    {
        std::cout << "Could not set backend (may not be supported)" << std::endl;
    }
}

}  // namespace xsigma::examples::smp

/**
 * @brief Main entry point for thread configuration examples.
 */
int main()
{
    std::cout << "XSigma SMP Examples - Thread Configuration" << std::endl;
    std::cout << "==========================================" << std::endl;

    try
    {
        xsigma::examples::smp::example_query_configuration();
        xsigma::examples::smp::example_initialize_thread_pool();
        xsigma::examples::smp::example_thread_count_performance();
        xsigma::examples::smp::example_nested_parallelism_control();
        xsigma::examples::smp::example_single_threaded_mode();
        xsigma::examples::smp::example_backend_selection();

        std::cout << "\n=== All Examples Completed Successfully ===" << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
