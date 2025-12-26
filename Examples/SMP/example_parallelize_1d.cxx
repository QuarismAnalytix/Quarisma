/**
 * @file example_parallelize_1d.cxx
 * @brief Demonstrates the usage of parallelize_1d() for high-performance data-parallel work
 *        distribution with work-stealing load balancing.
 *
 * This example shows:
 * - Basic parallelize_1d usage for simple data processing
 * - Work-stealing load balancing in action
 * - Performance comparison with sequential execution
 * - Real-world use case: vector operations
 */

#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

#include "smp_new/parallel/parallelize_1d.h"

namespace xsigma::examples::smp
{

/**
 * @brief Example 1: Basic vector initialization with parallelize_1d.
 *
 * Demonstrates the simplest use case: initializing a large vector in parallel.
 */
void example_basic_vector_init()
{
    std::cout << "\n=== Example 1: Basic Vector Initialization ===" << std::endl;

    const size_t       kVectorSize = 1000000;
    std::vector<float> data(kVectorSize);

    // Sequential initialization for comparison
    auto start_seq = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < kVectorSize; ++i)
    {
        data[i] = std::sin(i * 0.001f);
    }
    auto end_seq      = std::chrono::high_resolution_clock::now();
    auto seq_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_seq - start_seq);

    // Parallel initialization with parallelize_1d
    auto start_par = std::chrono::high_resolution_clock::now();
    xsigma::smp_new::parallel::parallelize_1d(
        [&data](size_t i) { data[i] = std::sin(i * 0.001f); }, kVectorSize);
    auto end_par      = std::chrono::high_resolution_clock::now();
    auto par_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_par - start_par);

    std::cout << "Vector size: " << kVectorSize << std::endl;
    std::cout << "Sequential time: " << seq_duration.count() << " ms" << std::endl;
    std::cout << "Parallel time:   " << par_duration.count() << " ms" << std::endl;
    std::cout << "Speedup: " << static_cast<float>(seq_duration.count()) / par_duration.count()
              << "x" << std::endl;
}

/**
 * @brief Example 2: Matrix row-wise operations with parallelize_1d.
 *
 * Demonstrates processing a 2D matrix where each row is processed independently.
 */
void example_matrix_operations()
{
    std::cout << "\n=== Example 2: Matrix Row-wise Operations ===" << std::endl;

    const size_t                    kRows = 1000;
    const size_t                    kCols = 1000;
    std::vector<std::vector<float>> matrix(kRows, std::vector<float>(kCols));

    // Initialize matrix in parallel
    xsigma::smp_new::parallel::parallelize_1d(
        [&matrix, kCols](size_t row)
        {
            for (size_t col = 0; col < kCols; ++col)
            {
                matrix[row][col] = std::sin(row * col * 0.0001f);
            }
        },
        kRows);

    // Compute row sums in parallel
    std::vector<float> row_sums(kRows, 0.0f);
    xsigma::smp_new::parallel::parallelize_1d(
        [&matrix, &row_sums, kCols](size_t row)
        {
            float sum = 0.0f;
            for (size_t col = 0; col < kCols; ++col)
            {
                sum += matrix[row][col];
            }
            row_sums[row] = sum;
        },
        kRows);

    std::cout << "Matrix size: " << kRows << "x" << kCols << std::endl;
    std::cout << "First row sum: " << row_sums[0] << std::endl;
    std::cout << "Last row sum:  " << row_sums[kRows - 1] << std::endl;
}

/**
 * @brief Example 3: Image processing simulation with parallelize_1d.
 *
 * Demonstrates processing image pixels in parallel (simulated with a 1D array).
 */
void example_image_processing()
{
    std::cout << "\n=== Example 3: Image Processing (Pixel-wise Operations) ===" << std::endl;

    const size_t         kImageWidth  = 1920;
    const size_t         kImageHeight = 1080;
    const size_t         kPixelCount  = kImageWidth * kImageHeight;
    std::vector<uint8_t> image(kPixelCount);

    // Simulate image processing: apply a simple filter
    xsigma::smp_new::parallel::parallelize_1d(
        [&image, kImageWidth](size_t pixel_idx)
        {
            // Simple edge detection simulation
            uint8_t value    = static_cast<uint8_t>((pixel_idx % 256));
            image[pixel_idx] = (value > 128) ? 255 : 0;
        },
        kPixelCount);

    std::cout << "Image size: " << kImageWidth << "x" << kImageHeight << " (" << kPixelCount
              << " pixels)" << std::endl;
    std::cout << "Processing completed successfully" << std::endl;
}

/**
 * @brief Example 4: Demonstrating work-stealing with uneven workload.
 *
 * Shows how parallelize_1d handles uneven work distribution through work-stealing.
 */
void example_uneven_workload()
{
    std::cout << "\n=== Example 4: Uneven Workload Distribution ===" << std::endl;

    const size_t        kWorkItems = 1000;
    std::vector<size_t> work_times(kWorkItems);

    // Create uneven workload: some items take longer than others
    xsigma::smp_new::parallel::parallelize_1d(
        [&work_times](size_t i)
        {
            // Simulate variable work: items divisible by 100 take longer
            size_t iterations = (i % 100 == 0) ? 10000 : 100;
            size_t result     = 0;
            for (size_t j = 0; j < iterations; ++j)
            {
                result += j * j;
            }
            work_times[i] = result;
        },
        kWorkItems);

    std::cout << "Processed " << kWorkItems << " items with uneven workload" << std::endl;
    std::cout << "Work-stealing load balancing handled the distribution" << std::endl;
}

/**
 * @brief Example 5: Data reduction with parallelize_1d.
 *
 * Demonstrates how to perform reductions (e.g., sum, min, max) in parallel.
 */
void example_data_reduction()
{
    std::cout << "\n=== Example 5: Data Reduction Operations ===" << std::endl;

    const size_t       kDataSize = 10000000;
    std::vector<float> data(kDataSize);

    // Initialize data
    xsigma::smp_new::parallel::parallelize_1d(
        [&data](size_t i) { data[i] = std::sin(i * 0.0001f); }, kDataSize);

    // Find minimum value in parallel (using atomic operations)
    std::atomic<float> min_value(std::numeric_limits<float>::max());
    xsigma::smp_new::parallel::parallelize_1d(
        [&data, &min_value](size_t i)
        {
            float current_min = min_value.load();
            while (data[i] < current_min && !min_value.compare_exchange_weak(current_min, data[i]))
            {
                current_min = min_value.load();
            }
        },
        kDataSize);

    std::cout << "Data size: " << kDataSize << std::endl;
    std::cout << "Minimum value: " << min_value.load() << std::endl;
}

}  // namespace xsigma::examples::smp

/**
 * @brief Main entry point for parallelize_1d examples.
 */
int main()
{
    std::cout << "XSigma SMP Examples - parallelize_1d()" << std::endl;
    std::cout << "======================================" << std::endl;

    try
    {
        xsigma::examples::smp::example_basic_vector_init();
        xsigma::examples::smp::example_matrix_operations();
        xsigma::examples::smp::example_image_processing();
        xsigma::examples::smp::example_uneven_workload();
        xsigma::examples::smp::example_data_reduction();

        std::cout << "\n=== All Examples Completed Successfully ===" << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
