/**
 * @file ASYNC_PARALLEL_EXAMPLES.cpp
 * @brief Example usage patterns for async_parallel_for and async_parallel_reduce
 *
 * This file contains comprehensive examples demonstrating how to use the
 * asynchronous parallel operations in XSigma. These examples are not compiled
 * as part of the build; they serve as documentation and reference.
 *
 * Compilation command (for testing):
 * g++ -std=c++17 -I../Library/Core -lpthread ASYNC_PARALLEL_EXAMPLES.cpp
 */

#include <algorithm>
#include <iostream>
#include <limits>
#include <vector>

// Include XSigma parallel headers
#include "parallel/async_handle.h"
#include "parallel/parallel.h"

// Example 1: Basic async_parallel_for usage
void example_basic_async_for()
{
    std::cout << "=== Example 1: Basic async_parallel_for ===" << std::endl;

    std::vector<int> data(10000);

    // Launch async parallel operation
    auto handle = xsigma::async_parallel_for(
        0,
        static_cast<int64_t>(data.size()),
        1000,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] = static_cast<int>(i * 2);
            }
        });

    std::cout << "Async operation launched, doing other work..." << std::endl;

    // Do other work while parallel operation runs
    for (int i = 0; i < 1000; ++i)
    {
        // Simulate other work
    }

    // Wait for completion
    handle.wait();

    if (handle.has_error())
    {
        std::cerr << "Error: " << handle.get_error() << std::endl;
    }
    else
    {
        std::cout << "Completed successfully! data[100] = " << data[100] << std::endl;
    }
}

// Example 2: Basic async_parallel_reduce usage
void example_basic_async_reduce()
{
    std::cout << "\n=== Example 2: Basic async_parallel_reduce ===" << std::endl;

    std::vector<int> data(10000, 1);  // All elements are 1

    // Launch async reduction
    auto handle = xsigma::async_parallel_reduce(
        0,
        static_cast<int64_t>(data.size()),
        2500,
        0,
        [&data](int64_t begin, int64_t end, int identity)
        {
            int partial_sum = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                partial_sum += data[i];
            }
            return partial_sum;
        },
        [](int a, int b) { return a + b; });

    std::cout << "Async reduction launched..." << std::endl;

    // Get result (blocks until ready)
    int sum = handle.get();

    if (handle.has_error())
    {
        std::cerr << "Error: " << handle.get_error() << std::endl;
    }
    else
    {
        std::cout << "Sum: " << sum << " (expected: 10000)" << std::endl;
    }
}

// Example 3: Multiple concurrent async operations
void example_concurrent_operations()
{
    std::cout << "\n=== Example 3: Multiple concurrent operations ===" << std::endl;

    const size_t        N = 100000;
    std::vector<double> data1(N), data2(N), data3(N);
    std::vector<double> results1(N), results2(N), results3(N);

    // Initialize data
    for (size_t i = 0; i < N; ++i)
    {
        data1[i] = static_cast<double>(i);
        data2[i] = static_cast<double>(i) * 2.0;
        data3[i] = static_cast<double>(i) * 3.0;
    }

    // Launch three concurrent operations
    auto handle1 = xsigma::async_parallel_for(
        0,
        static_cast<int64_t>(N),
        10000,
        [&](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                results1[i] = data1[i] * data1[i];  // Square
            }
        });

    auto handle2 = xsigma::async_parallel_for(
        0,
        static_cast<int64_t>(N),
        10000,
        [&](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                results2[i] = data2[i] * data2[i];  // Square
            }
        });

    auto handle3 = xsigma::async_parallel_for(
        0,
        static_cast<int64_t>(N),
        10000,
        [&](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                results3[i] = data3[i] * data3[i];  // Square
            }
        });

    std::cout << "Three async operations launched, waiting..." << std::endl;

    // Wait for all operations
    handle1.wait();
    handle2.wait();
    handle3.wait();

    if (handle1.has_error() || handle2.has_error() || handle3.has_error())
    {
        std::cerr << "At least one operation failed" << std::endl;
    }
    else
    {
        std::cout << "All operations completed successfully" << std::endl;
        std::cout << "results1[100] = " << results1[100] << " (expected: 10000)" << std::endl;
    }
}

// Example 4: Concurrent reductions (compute multiple statistics)
void example_concurrent_reductions()
{
    std::cout << "\n=== Example 4: Concurrent reductions ===" << std::endl;

    std::vector<double> data(1000000);
    for (size_t i = 0; i < data.size(); ++i)
    {
        data[i] = static_cast<double>(i % 100);
    }

    // Launch sum reduction
    auto sum_handle = xsigma::async_parallel_reduce(
        0,
        static_cast<int64_t>(data.size()),
        10000,
        0.0,
        [&data](int64_t begin, int64_t end, double identity)
        {
            double partial_sum = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                partial_sum += data[i];
            }
            return partial_sum;
        },
        [](double a, double b) { return a + b; });

    // Launch max reduction
    auto max_handle = xsigma::async_parallel_reduce(
        0,
        static_cast<int64_t>(data.size()),
        10000,
        std::numeric_limits<double>::lowest(),
        [&data](int64_t begin, int64_t end, double identity)
        {
            double partial_max = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                partial_max = std::max(partial_max, data[i]);
            }
            return partial_max;
        },
        [](double a, double b) { return std::max(a, b); });

    // Launch min reduction
    auto min_handle = xsigma::async_parallel_reduce(
        0,
        static_cast<int64_t>(data.size()),
        10000,
        std::numeric_limits<double>::max(),
        [&data](int64_t begin, int64_t end, double identity)
        {
            double partial_min = identity;
            for (int64_t i = begin; i < end; ++i)
            {
                partial_min = std::min(partial_min, data[i]);
            }
            return partial_min;
        },
        [](double a, double b) { return std::min(a, b); });

    std::cout << "Three reductions launched concurrently..." << std::endl;

    // Get results (all run concurrently)
    double sum     = sum_handle.get();
    double max_val = max_handle.get();
    double min_val = min_handle.get();

    if (!sum_handle.has_error() && !max_handle.has_error() && !min_handle.has_error())
    {
        double mean = sum / data.size();
        std::cout << "Sum: " << sum << std::endl;
        std::cout << "Mean: " << mean << std::endl;
        std::cout << "Max: " << max_val << std::endl;
        std::cout << "Min: " << min_val << std::endl;
    }
    else
    {
        std::cerr << "One or more reductions failed" << std::endl;
    }
}

// Example 5: Timeout and error handling
void example_timeout_handling()
{
    std::cout << "\n=== Example 5: Timeout and error handling ===" << std::endl;

    std::vector<int> data(1000000);

    // Launch a long-running operation
    auto handle = xsigma::async_parallel_for(
        0,
        static_cast<int64_t>(data.size()),
        100,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                // Simulate expensive computation
                double result = 0.0;
                for (int j = 0; j < 1000; ++j)
                {
                    result += std::sin(static_cast<double>(i + j));
                }
                data[i] = static_cast<int>(result);
            }
        });

    // Wait with timeout (100ms)
    if (handle.wait_for(100))
    {
        std::cout << "Operation completed within 100ms" << std::endl;
    }
    else
    {
        std::cout << "Operation did not complete within 100ms, still waiting..." << std::endl;
        handle.wait();  // Wait indefinitely
        std::cout << "Operation completed" << std::endl;
    }

    if (handle.has_error())
    {
        std::cerr << "Error: " << handle.get_error() << std::endl;
    }
    else
    {
        std::cout << "Completed successfully" << std::endl;
    }
}

// Example 6: Batch processing pattern
void example_batch_processing()
{
    std::cout << "\n=== Example 6: Batch processing pattern ===" << std::endl;

    const size_t NUM_BATCHES = 10;
    const size_t BATCH_SIZE  = 100000;

    std::vector<std::vector<double>> batches(NUM_BATCHES);
    for (auto& batch : batches)
    {
        batch.resize(BATCH_SIZE);
    }

    // Store handles for all batch operations
    std::vector<xsigma::async_handle<void>> handles;
    handles.reserve(NUM_BATCHES);

    // Launch all batch operations
    for (size_t batch_idx = 0; batch_idx < NUM_BATCHES; ++batch_idx)
    {
        handles.emplace_back(xsigma::async_parallel_for(
            0,
            static_cast<int64_t>(BATCH_SIZE),
            10000,
            [&batches, batch_idx](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    batches[batch_idx][i] = std::exp(static_cast<double>(i) / 1000.0);
                }
            }));
    }

    std::cout << NUM_BATCHES << " batch operations launched" << std::endl;

    // Wait for all batches to complete
    size_t completed = 0;
    for (auto& handle : handles)
    {
        handle.wait();
        if (!handle.has_error())
        {
            ++completed;
        }
    }

    std::cout << completed << " / " << NUM_BATCHES << " batches completed successfully"
              << std::endl;
}

// Example 7: Pipelined processing
void example_pipelined_processing()
{
    std::cout << "\n=== Example 7: Pipelined processing ===" << std::endl;

    const size_t        N = 1000000;
    std::vector<double> input(N);
    std::vector<double> stage1_output(N);
    std::vector<double> stage2_output(N);
    std::vector<double> final_output(N);

    // Initialize input
    for (size_t i = 0; i < N; ++i)
    {
        input[i] = static_cast<double>(i);
    }

    // Stage 1: Preprocessing
    auto stage1_handle = xsigma::async_parallel_for(
        0,
        static_cast<int64_t>(N),
        10000,
        [&](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                stage1_output[i] = std::sqrt(input[i]);
            }
        });

    stage1_handle.wait();
    std::cout << "Stage 1 complete" << std::endl;

    // Stage 2: Main processing (depends on stage 1)
    auto stage2_handle = xsigma::async_parallel_for(
        0,
        static_cast<int64_t>(N),
        10000,
        [&](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                stage2_output[i] = stage1_output[i] * stage1_output[i] + 1.0;
            }
        });

    stage2_handle.wait();
    std::cout << "Stage 2 complete" << std::endl;

    // Stage 3: Postprocessing (depends on stage 2)
    auto stage3_handle = xsigma::async_parallel_for(
        0,
        static_cast<int64_t>(N),
        10000,
        [&](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                final_output[i] = std::log(stage2_output[i]);
            }
        });

    stage3_handle.wait();
    std::cout << "Stage 3 complete" << std::endl;

    if (!stage1_handle.has_error() && !stage2_handle.has_error() && !stage3_handle.has_error())
    {
        std::cout << "Pipeline completed successfully" << std::endl;
        std::cout << "final_output[1000] = " << final_output[1000] << std::endl;
    }
}

// Example 8: Background computation pattern
void example_background_computation()
{
    std::cout << "\n=== Example 8: Background computation ===" << std::endl;

    const size_t        N = 10000000;
    std::vector<double> data(N);
    std::vector<double> results(N);

    // Initialize data
    for (size_t i = 0; i < N; ++i)
    {
        data[i] = static_cast<double>(i);
    }

    // Launch expensive computation in background
    auto compute_handle = xsigma::async_parallel_for(
        0,
        static_cast<int64_t>(N),
        100000,
        [&](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                results[i] = std::exp(data[i] / 1000000.0);
            }
        });

    std::cout << "Background computation launched" << std::endl;

    // Do other work while computation runs
    std::cout << "Doing other work:" << std::endl;
    std::cout << "  - Preparing output directory..." << std::endl;
    std::cout << "  - Validating inputs..." << std::endl;
    std::cout << "  - Loading configuration..." << std::endl;

    // Wait for computation to finish
    compute_handle.wait();

    if (compute_handle.has_error())
    {
        std::cerr << "Background computation failed: " << compute_handle.get_error() << std::endl;
    }
    else
    {
        std::cout << "Background computation completed, results ready" << std::endl;
    }
}

// Example 9: Using is_ready() for polling
void example_polling_pattern()
{
    std::cout << "\n=== Example 9: Polling pattern ===" << std::endl;

    std::vector<int> data(1000000);

    auto handle = xsigma::async_parallel_for(
        0,
        static_cast<int64_t>(data.size()),
        10000,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] = static_cast<int>(i);
            }
        });

    // Poll for completion
    int poll_count = 0;
    while (!handle.is_ready())
    {
        ++poll_count;
        // Do some work while waiting
        for (int i = 0; i < 1000000; ++i)
        {
            volatile int dummy = i;  // Prevent optimization
            (void)dummy;
        }
    }

    std::cout << "Operation completed after " << poll_count << " polls" << std::endl;
}

int main()
{
    std::cout << "XSigma Async Parallel Operations Examples\n" << std::endl;

    // Run all examples
    example_basic_async_for();
    example_basic_async_reduce();
    example_concurrent_operations();
    example_concurrent_reductions();
    example_timeout_handling();
    example_batch_processing();
    example_pipelined_processing();
    example_background_computation();
    example_polling_pattern();

    std::cout << "\n=== All examples completed ===" << std::endl;

    return 0;
}
