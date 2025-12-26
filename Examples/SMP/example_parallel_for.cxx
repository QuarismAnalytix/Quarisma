/**
 * @file example_parallel_for.cxx
 * @brief Demonstrates the usage of parallel_for() for chunk-based parallel iteration.
 *
 * This example shows:
 * - Basic parallel_for usage with different grain sizes
 * - Chunk-based work distribution
 * - Nested parallelism capabilities
 * - Real-world use cases: range-based processing
 */

#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

#include "smp_new/parallel/parallel_api.h"

namespace xsigma::examples::smp
{

/**
 * @brief Example 1: Basic parallel_for with automatic grain size.
 *
 * Demonstrates the simplest use case with automatic grain size determination.
 */
void example_basic_parallel_for()
{
    std::cout << "\n=== Example 1: Basic parallel_for ===" << std::endl;

    const int64_t      kRangeStart = 0;
    const int64_t      kRangeEnd   = 1000000;
    std::vector<float> results(kRangeEnd - kRangeStart);

    // Parallel iteration with automatic grain size
    xsigma::smp_new::parallel::parallel_for(
        kRangeStart,
        kRangeEnd,
        -1,  // -1 means automatic grain size
        [&results](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                results[i] = std::sin(i * 0.001f);
            }
        });

    std::cout << "Processed range: [" << kRangeStart << ", " << kRangeEnd << ")" << std::endl;
    std::cout << "First result: " << results[0] << std::endl;
    std::cout << "Last result:  " << results[kRangeEnd - kRangeStart - 1] << std::endl;
}

/**
 * @brief Example 2: parallel_for with explicit grain size.
 *
 * Demonstrates how grain size affects work distribution and performance.
 */
void example_grain_size_tuning()
{
    std::cout << "\n=== Example 2: Grain Size Tuning ===" << std::endl;

    const int64_t      kRangeStart = 0;
    const int64_t      kRangeEnd   = 10000000;
    std::vector<float> results(kRangeEnd - kRangeStart);

    // Test different grain sizes
    std::vector<int64_t> grain_sizes = {1000, 10000, 100000, 1000000};

    for (int64_t grain_size : grain_sizes)
    {
        auto start = std::chrono::high_resolution_clock::now();

        xsigma::smp_new::parallel::parallel_for(
            kRangeStart,
            kRangeEnd,
            grain_size,
            [&results](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    results[i] = std::sin(i * 0.001f);
                }
            });

        auto end      = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "Grain size " << grain_size << ": " << duration.count() << " ms" << std::endl;
    }
}

/**
 * @brief Example 3: Chunk-based matrix multiplication.
 *
 * Demonstrates processing chunks of data in parallel.
 */
void example_chunk_based_processing()
{
    std::cout << "\n=== Example 3: Chunk-based Matrix Processing ===" << std::endl;

    const int64_t                   kMatrixSize = 1000;
    std::vector<std::vector<float>> matrix(kMatrixSize, std::vector<float>(kMatrixSize));

    // Process matrix rows in chunks
    xsigma::smp_new::parallel::parallel_for(
        0,
        static_cast<int64_t>(kMatrixSize),
        100,  // Process 100 rows per chunk
        [&matrix, kMatrixSize](int64_t begin, int64_t end)
        {
            for (int64_t row = begin; row < end; ++row)
            {
                for (int64_t col = 0; col < kMatrixSize; ++col)
                {
                    matrix[row][col] = std::sin(row * col * 0.0001f);
                }
            }
        });

    std::cout << "Matrix size: " << kMatrixSize << "x" << kMatrixSize << std::endl;
    std::cout << "Processed in chunks of 100 rows" << std::endl;
}

/**
 * @brief Example 4: Nested parallelism with parallel_for.
 *
 * Demonstrates nested parallel regions (outer parallel_for with inner parallel_for).
 */
void example_nested_parallelism()
{
    std::cout << "\n=== Example 4: Nested Parallelism ===" << std::endl;

    const int64_t                   kOuterSize = 100;
    const int64_t                   kInnerSize = 1000;
    std::vector<std::vector<float>> data(kOuterSize, std::vector<float>(kInnerSize));

    // Outer parallel loop
    xsigma::smp_new::parallel::parallel_for(
        0,
        kOuterSize,
        10,  // Process 10 outer iterations per chunk
        [&data, kInnerSize](int64_t outer_begin, int64_t outer_end)
        {
            // Inner parallel loop (nested)
            for (int64_t i = outer_begin; i < outer_end; ++i)
            {
                xsigma::smp_new::parallel::parallel_for(
                    0,
                    kInnerSize,
                    100,  // Process 100 inner iterations per chunk
                    [&data, i](int64_t inner_begin, int64_t inner_end)
                    {
                        for (int64_t j = inner_begin; j < inner_end; ++j)
                        {
                            data[i][j] = std::sin(i * j * 0.0001f);
                        }
                    });
            }
        });

    std::cout << "Outer size: " << kOuterSize << ", Inner size: " << kInnerSize << std::endl;
    std::cout << "Nested parallelism completed" << std::endl;
}

/**
 * @brief Example 5: Reduction with parallel_for.
 *
 * Demonstrates how to perform reductions using parallel_for with thread-local storage.
 */
void example_parallel_reduction()
{
    std::cout << "\n=== Example 5: Parallel Reduction ===" << std::endl;

    const int64_t      kDataSize = 10000000;
    std::vector<float> data(kDataSize);

    // Initialize data
    xsigma::smp_new::parallel::parallel_for(
        0,
        kDataSize,
        100000,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] = std::sin(i * 0.0001f);
            }
        });

    // Compute sum using parallel_for with atomic operations
    std::atomic<double> total_sum(0.0);
    xsigma::smp_new::parallel::parallel_for(
        0,
        kDataSize,
        100000,
        [&data, &total_sum](int64_t begin, int64_t end)
        {
            // Use std::accumulate for summing range
            double const local_sum = std::accumulate(data.begin() + begin, data.begin() + end, 0.0);
            // Atomic update
            double current = total_sum.load();
            while (!total_sum.compare_exchange_weak(current, current + local_sum))
            {
                current = total_sum.load();
            }
        });

    std::cout << "Data size: " << kDataSize << std::endl;
    std::cout << "Total sum: " << total_sum.load() << std::endl;
}

/**
 * @brief Example 6: Comparison of grain sizes on performance.
 *
 * Demonstrates the impact of grain size on performance and load balancing.
 */
void example_performance_analysis()
{
    std::cout << "\n=== Example 6: Performance Analysis ===" << std::endl;

    const int64_t      kRangeSize = 100000000;
    std::vector<float> results(kRangeSize);

    // Small grain size (more overhead, better load balancing)
    auto start1 = std::chrono::high_resolution_clock::now();
    xsigma::smp_new::parallel::parallel_for(
        0,
        kRangeSize,
        1000,
        [&results](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                results[i] = std::sin(i * 0.0001f);
            }
        });
    auto end1      = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);

    // Large grain size (less overhead, potential load imbalance)
    auto start2 = std::chrono::high_resolution_clock::now();
    xsigma::smp_new::parallel::parallel_for(
        0,
        kRangeSize,
        1000000,
        [&results](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                results[i] = std::sin(i * 0.0001f);
            }
        });
    auto end2      = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);

    std::cout << "Range size: " << kRangeSize << std::endl;
    std::cout << "Small grain size (1000):    " << duration1.count() << " ms" << std::endl;
    std::cout << "Large grain size (1000000): " << duration2.count() << " ms" << std::endl;
}

}  // namespace xsigma::examples::smp

/**
 * @brief Main entry point for parallel_for examples.
 */
int main()
{
    std::cout << "XSigma SMP Examples - parallel_for()" << std::endl;
    std::cout << "====================================" << std::endl;

    try
    {
        xsigma::examples::smp::example_basic_parallel_for();
        xsigma::examples::smp::example_grain_size_tuning();
        xsigma::examples::smp::example_chunk_based_processing();
        xsigma::examples::smp::example_nested_parallelism();
        xsigma::examples::smp::example_parallel_reduction();
        xsigma::examples::smp::example_performance_analysis();

        std::cout << "\n=== All Examples Completed Successfully ===" << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
