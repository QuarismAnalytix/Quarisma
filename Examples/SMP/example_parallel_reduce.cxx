/**
 * @file example_parallel_reduce.cxx
 * @brief Demonstrates the usage of parallel_reduce() for efficient reduction operations.
 *
 * This example shows:
 * - Basic parallel reduction (sum, min, max)
 * - Custom reduction functors
 * - Combining results from multiple threads
 * - Real-world use cases: aggregations, statistics
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>

#include "smp_new/parallel/parallel_api.h"

namespace xsigma::examples::smp
{

/**
 * @brief Example 1: Basic sum reduction.
 *
 * Demonstrates the simplest reduction operation: computing a sum.
 */
void example_basic_sum_reduction()
{
    std::cout << "\n=== Example 1: Basic Sum Reduction ===" << std::endl;

    const size_t       kDataSize = 10000000;
    std::vector<float> data(kDataSize);

    // Initialize data
    for (size_t i = 0; i < kDataSize; ++i)
    {
        data[i] = std::sin(i * 0.0001f);
    }

    // Sequential sum for comparison using std::accumulate
    auto  start_seq    = std::chrono::high_resolution_clock::now();
    float seq_sum      = std::accumulate(data.begin(), data.end(), 0.0f);
    auto  end_seq      = std::chrono::high_resolution_clock::now();
    auto  seq_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_seq - start_seq);

    // Parallel sum using parallel_reduce
    auto  start_par = std::chrono::high_resolution_clock::now();
    float par_sum   = xsigma::smp_new::parallel::parallel_reduce(
        0,
        static_cast<int64_t>(kDataSize),
        0.0f,
        [&data](int64_t begin, int64_t end, float init)
        {
            // Use std::accumulate for summing range
            return std::accumulate(data.begin() + begin, data.begin() + end, init);
        },
        [](float a, float b) { return a + b; });
    auto end_par      = std::chrono::high_resolution_clock::now();
    auto par_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_par - start_par);

    std::cout << "Data size: " << kDataSize << std::endl;
    std::cout << "Sequential sum: " << seq_sum << " (" << seq_duration.count() << " ms)"
              << std::endl;
    std::cout << "Parallel sum:   " << par_sum << " (" << par_duration.count() << " ms)"
              << std::endl;
    std::cout << "Speedup: " << static_cast<float>(seq_duration.count()) / par_duration.count()
              << "x" << std::endl;
}

/**
 * @brief Example 2: Min/Max reduction.
 *
 * Demonstrates finding minimum and maximum values in parallel.
 */
void example_min_max_reduction()
{
    std::cout << "\n=== Example 2: Min/Max Reduction ===" << std::endl;

    const size_t       kDataSize = 10000000;
    std::vector<float> data(kDataSize);

    // Initialize data with random-like values
    for (size_t i = 0; i < kDataSize; ++i)
    {
        data[i] = std::sin(i * 0.0001f) * 100.0f;
    }

    // Find minimum
    float min_value = xsigma::smp_new::parallel::parallel_reduce(
        0,
        static_cast<int64_t>(kDataSize),
        std::numeric_limits<float>::max(),
        [&data](int64_t begin, int64_t end, float init)
        {
            // Use std::min_element for finding minimum
            auto const it = std::min_element(data.begin() + begin, data.begin() + end);
            return (it != data.begin() + end) ? std::min(init, *it) : init;
        },
        [](float a, float b) { return std::min(a, b); });

    // Find maximum
    float max_value = xsigma::smp_new::parallel::parallel_reduce(
        0,
        static_cast<int64_t>(kDataSize),
        std::numeric_limits<float>::lowest(),
        [&data](int64_t begin, int64_t end, float init)
        {
            // Use std::max_element for finding maximum
            auto const it = std::max_element(data.begin() + begin, data.begin() + end);
            return (it != data.begin() + end) ? std::max(init, *it) : init;
        },
        [](float a, float b) { return std::max(a, b); });

    std::cout << "Data size: " << kDataSize << std::endl;
    std::cout << "Minimum value: " << min_value << std::endl;
    std::cout << "Maximum value: " << max_value << std::endl;
}

/**
 * @brief Example 3: Custom reduction with struct.
 *
 * Demonstrates reduction with custom data types (e.g., computing statistics).
 */
void example_custom_reduction()
{
    std::cout << "\n=== Example 3: Custom Reduction (Statistics) ===" << std::endl;

    struct statistics
    {
        double sum    = 0.0;
        double sum_sq = 0.0;
        size_t count  = 0;
    };

    const size_t       kDataSize = 10000000;
    std::vector<float> data(kDataSize);

    // Initialize data
    for (size_t i = 0; i < kDataSize; ++i)
    {
        data[i] = std::sin(i * 0.0001f);
    }

    // Compute statistics in parallel
    statistics stats = xsigma::smp_new::parallel::parallel_reduce(
        0,
        static_cast<int64_t>(kDataSize),
        statistics(),
        [&data](int64_t begin, int64_t end, statistics init)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                init.sum += data[i];
                init.sum_sq += data[i] * data[i];
                init.count++;
            }
            return init;
        },
        [](const statistics& a, const statistics& b)
        { return statistics{a.sum + b.sum, a.sum_sq + b.sum_sq, a.count + b.count}; });

    double mean     = stats.sum / stats.count;
    double variance = (stats.sum_sq / stats.count) - (mean * mean);
    double stddev   = std::sqrt(variance);

    std::cout << "Data size: " << stats.count << std::endl;
    std::cout << "Mean: " << mean << std::endl;
    std::cout << "Variance: " << variance << std::endl;
    std::cout << "Standard deviation: " << stddev << std::endl;
}

/**
 * @brief Example 4: Vector reduction (combining vectors).
 *
 * Demonstrates reducing multiple vectors into a single result.
 */
void example_vector_reduction()
{
    std::cout << "\n=== Example 4: Vector Reduction ===" << std::endl;

    const size_t                    kVectorSize = 1000;
    const size_t                    kNumVectors = 10000;
    std::vector<std::vector<float>> vectors(kNumVectors, std::vector<float>(kVectorSize));

    // Initialize vectors
    for (size_t i = 0; i < kNumVectors; ++i)
    {
        for (size_t j = 0; j < kVectorSize; ++j)
        {
            vectors[i][j] = std::sin((i + j) * 0.001f);
        }
    }

    // Reduce vectors: compute element-wise sum
    std::vector<float> result = xsigma::smp_new::parallel::parallel_reduce(
        0,
        static_cast<int64_t>(kNumVectors),
        std::vector<float>(kVectorSize, 0.0f),
        [&vectors, kVectorSize](int64_t begin, int64_t end, std::vector<float> init)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                for (size_t j = 0; j < kVectorSize; ++j)
                {
                    init[j] += vectors[i][j];
                }
            }
            return init;
        },
        [kVectorSize](const std::vector<float>& a, const std::vector<float>& b)
        {
            std::vector<float> result(kVectorSize);
            for (size_t i = 0; i < kVectorSize; ++i)
            {
                result[i] = a[i] + b[i];
            }
            return result;
        });

    std::cout << "Number of vectors: " << kNumVectors << std::endl;
    std::cout << "Vector size: " << kVectorSize << std::endl;
    std::cout << "First element of result: " << result[0] << std::endl;
    std::cout << "Last element of result:  " << result[kVectorSize - 1] << std::endl;
}

/**
 * @brief Example 5: Histogram computation with reduction.
 *
 * Demonstrates computing a histogram in parallel using reduction.
 */
void example_histogram_reduction()
{
    std::cout << "\n=== Example 5: Histogram Computation ===" << std::endl;

    const size_t       kDataSize = 10000000;
    const size_t       kBuckets  = 100;
    std::vector<float> data(kDataSize);

    // Initialize data with values in range [0, 1)
    for (size_t i = 0; i < kDataSize; ++i)
    {
        data[i] = std::fmod(std::sin(i * 0.0001f) + 1.0f, 1.0f);
    }

    // Compute histogram in parallel
    std::vector<size_t> histogram = xsigma::smp_new::parallel::parallel_reduce(
        0,
        static_cast<int64_t>(kDataSize),
        std::vector<size_t>(kBuckets, 0),
        [&data, kBuckets](int64_t begin, int64_t end, std::vector<size_t> init)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                size_t bucket = static_cast<size_t>(data[i] * kBuckets);
                if (bucket >= kBuckets)
                    bucket = kBuckets - 1;
                init[bucket]++;
            }
            return init;
        },
        [kBuckets](const std::vector<size_t>& a, const std::vector<size_t>& b)
        {
            std::vector<size_t> result(kBuckets);
            for (size_t i = 0; i < kBuckets; ++i)
            {
                result[i] = a[i] + b[i];
            }
            return result;
        });

    std::cout << "Data size: " << kDataSize << std::endl;
    std::cout << "Number of buckets: " << kBuckets << std::endl;
    std::cout << "First bucket count: " << histogram[0] << std::endl;
    std::cout << "Last bucket count:  " << histogram[kBuckets - 1] << std::endl;
}

}  // namespace xsigma::examples::smp

/**
 * @brief Main entry point for parallel_reduce examples.
 */
int main()
{
    std::cout << "XSigma SMP Examples - parallel_reduce()" << std::endl;
    std::cout << "=======================================" << std::endl;

    try
    {
        xsigma::examples::smp::example_basic_sum_reduction();
        xsigma::examples::smp::example_min_max_reduction();
        xsigma::examples::smp::example_custom_reduction();
        xsigma::examples::smp::example_vector_reduction();
        xsigma::examples::smp::example_histogram_reduction();

        std::cout << "\n=== All Examples Completed Successfully ===" << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
