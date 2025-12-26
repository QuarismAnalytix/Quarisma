/**
 * @file example_profiling_basic.cxx
 * @brief Comprehensive example demonstrating XSigma's profiling systems.
 *
 * This example shows:
 * - XSigma native profiler with hierarchical CPU profiling
 * - Kineto profiler for GPU-related CPU operations
 * - ITT profiler for Intel VTune integration
 * - Combined profiling with multiple systems
 * - Graceful degradation when profilers are unavailable
 * - Best practices for profiling instrumentation
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

#include "profiler/session/profiler.h"

#if XSIGMA_HAS_KINETO
#include "profiler/kineto_shim.h"
#include <ActivityTrace.h>
#endif

#if XSIGMA_HAS_ITT
#include "profiler/itt_wrapper.h"
#endif

namespace xsigma::examples::profiling
{

// ============================================================================
// Helper Functions - Computational Workloads
// ============================================================================

/**
 * @brief Matrix multiplication with profiling instrumentation.
 */
std::vector<std::vector<double>> matrix_multiply(
    const std::vector<std::vector<double>>& a,
    const std::vector<std::vector<double>>& b)
{
    XSIGMA_PROFILE_SCOPE("matrix_multiply");

    const size_t rows_a = a.size();
    const size_t cols_a = a[0].size();
    const size_t cols_b = b[0].size();

    std::vector<std::vector<double>> result(rows_a, std::vector<double>(cols_b, 0.0));

    {
        XSIGMA_PROFILE_SCOPE("matrix_multiply_computation");

        for (size_t i = 0; i < rows_a; ++i)
        {
            for (size_t j = 0; j < cols_b; ++j)
            {
                for (size_t k = 0; k < cols_a; ++k)
                {
                    result[i][j] += a[i][k] * b[k][j];
                }
            }
        }
    }

    return result;
}

/**
 * @brief Generate a random matrix for testing.
 */
std::vector<std::vector<double>> generate_matrix(size_t rows, size_t cols)
{
    XSIGMA_PROFILE_SCOPE("generate_matrix");

    std::vector<std::vector<double>> matrix(rows, std::vector<double>(cols));

    std::random_device                     rd;
    std::mt19937                           gen(rd());
    std::uniform_real_distribution<double> dis(0.0, 1.0);

    for (size_t i = 0; i < rows; ++i)
    {
        for (size_t j = 0; j < cols; ++j)
        {
            matrix[i][j] = dis(gen);
        }
    }

    return matrix;
}

/**
 * @brief Sorting algorithm with profiling.
 */
void merge_sort(std::vector<double>& arr, size_t left, size_t right)
{
    XSIGMA_PROFILE_SCOPE("merge_sort");

    if (left >= right)
        return;

    size_t mid = left + (right - left) / 2;

    merge_sort(arr, left, mid);
    merge_sort(arr, mid + 1, right);

    // Merge step
    std::vector<double> temp(right - left + 1);
    size_t              i = left, j = mid + 1, k = 0;

    while (i <= mid && j <= right)
    {
        if (arr[i] <= arr[j])
        {
            temp[k++] = arr[i++];
        }
        else
        {
            temp[k++] = arr[j++];
        }
    }

    while (i <= mid)
        temp[k++] = arr[i++];
    while (j <= right)
        temp[k++] = arr[j++];

    for (size_t idx = 0; idx < temp.size(); ++idx)
    {
        arr[left + idx] = temp[idx];
    }
}

// ============================================================================
// Example 1: XSigma Native Profiler
// ============================================================================

/**
 * @brief Demonstrates XSigma's native profiler with hierarchical CPU profiling.
 *
 * The native profiler provides:
 * - Hierarchical scope tracking with XSIGMA_PROFILE_SCOPE()
 * - Chrome Trace JSON export for visualization
 * - Full drill-down capability in Chrome DevTools and Perfetto UI
 */
void example_xsigma_native_profiler()
{
    std::cout << "\n=== Example 1: XSigma Native Profiler ===" << std::endl;

    // Configure profiler options
    profiler_options opts;
    opts.enable_timing_               = true;   // Enable timing measurements
    opts.enable_memory_tracking_      = false;  // Disable memory tracking for this example
    opts.enable_statistical_analysis_ = false;  // Disable statistics
    opts.enable_thread_safety_        = true;   // Thread-safe operations
    opts.output_format_               = profiler_options::output_format_enum::JSON;

    // Create and start profiler session
    profiler_session session(opts);
    session.start();

    std::cout << "✓ XSigma profiler started" << std::endl;

    // Profile matrix operations
    {
        XSIGMA_PROFILE_SCOPE("matrix_operations");

        const size_t matrix_size = 100;
        auto         matrix_a    = generate_matrix(matrix_size, matrix_size);
        auto         matrix_b    = generate_matrix(matrix_size, matrix_size);

        auto result = matrix_multiply(matrix_a, matrix_b);

        std::cout << "  Matrix multiplication completed (" << matrix_size << "x" << matrix_size
                  << ")" << std::endl;
    }

    // Profile sorting operations
    {
        XSIGMA_PROFILE_SCOPE("sorting_operations");

        const size_t        array_size = 10000;
        std::vector<double> test_data(array_size);

        std::random_device                     rd;
        std::mt19937                           gen(rd());
        std::uniform_real_distribution<double> dis(0.0, 1000.0);

        for (size_t i = 0; i < array_size; ++i)
        {
            test_data[i] = dis(gen);
        }

        merge_sort(test_data, 0, test_data.size() - 1);

        std::cout << "  Sorting completed (" << array_size << " elements)" << std::endl;
    }

    // Stop profiling
    session.stop();

    // Export Chrome Trace JSON
    std::string const output_file = "xsigma_native_profile.json";
    session.write_chrome_trace(output_file);

    std::cout << "✓ XSigma profiler stopped" << std::endl;
    std::cout << "✓ Trace saved to: " << output_file << std::endl;
    std::cout << "\nVisualization:" << std::endl;
    std::cout << "  1. Chrome DevTools: chrome://tracing" << std::endl;
    std::cout << "  2. Perfetto UI: https://ui.perfetto.dev" << std::endl;
}

// ============================================================================
// Example 2: Kineto Profiler
// ============================================================================

#if XSIGMA_HAS_KINETO

/**
 * @brief Demonstrates Kineto profiler combined with XSigma profiler.
 *
 * Kineto captures GPU-related CPU operations. For hierarchical CPU profiling,
 * we combine it with XSigma's native profiler.
 */
void example_kineto_profiler()
{
    std::cout << "\n=== Example 2: Kineto Profiler ===" << std::endl;

    // Initialize Kineto profiler
    xsigma::profiler::kineto_init(false, true);

    if (!xsigma::profiler::kineto_is_profiler_registered())
    {
        std::cout << "✗ Kineto profiler not available - skipping example" << std::endl;
        return;
    }

    std::cout << "✓ Kineto profiler initialized" << std::endl;

    // Prepare Kineto trace
    std::set<libkineto::ActivityType> activities;
    activities.insert(libkineto::ActivityType::CPU_OP);
    xsigma::profiler::kineto_prepare_trace(activities);

    // Start Kineto profiling
    xsigma::profiler::kineto_start_trace();

    // Also start XSigma profiler for hierarchical CPU profiling
    profiler_options opts;
    opts.enable_timing_  = true;
    opts.output_format_  = profiler_options::output_format_enum::JSON;

    profiler_session session(opts);
    session.start();

    std::cout << "✓ Combined profiling started (Kineto + XSigma)" << std::endl;

    // Profile workload
    {
        XSIGMA_PROFILE_SCOPE("kineto_workload");

        const size_t matrix_size = 80;
        auto         matrix_a    = generate_matrix(matrix_size, matrix_size);
        auto         matrix_b    = generate_matrix(matrix_size, matrix_size);

        auto result = matrix_multiply(matrix_a, matrix_b);

        std::cout << "  Workload completed" << std::endl;
    }

    // Stop both profilers
    session.stop();

    std::unique_ptr<libkineto::ActivityTraceInterface> kineto_trace(
        static_cast<libkineto::ActivityTraceInterface*>(xsigma::profiler::kineto_stop_trace()));

    // Export traces
    std::string const xsigma_file = "kineto_xsigma_trace.json";
    std::string const kineto_file = "kineto_only_trace.json";

    session.write_chrome_trace(xsigma_file);

    if (kineto_trace)
    {
        kineto_trace->save(kineto_file);
    }

    std::cout << "✓ Combined profiling stopped" << std::endl;
    std::cout << "✓ XSigma trace saved to: " << xsigma_file << std::endl;
    std::cout << "✓ Kineto trace saved to: " << kineto_file << std::endl;
}

#endif  // XSIGMA_HAS_KINETO

// ============================================================================
// Example 3: ITT Profiler
// ============================================================================

#if XSIGMA_HAS_ITT

/**
 * @brief Demonstrates ITT profiler combined with XSigma profiler.
 *
 * ITT provides annotations for Intel VTune Profiler. We combine it with
 * XSigma's profiler for JSON export and graceful degradation.
 */
void example_itt_profiler()
{
    std::cout << "\n=== Example 3: ITT Profiler ===" << std::endl;

    // Initialize ITT profiler
    xsigma::profiler::itt_init();

    // Check if ITT is available
    bool const itt_available = (xsigma::profiler::itt_get_domain() != nullptr);

    if (!itt_available)
    {
        std::cout << "✗ ITT not available (VTune not installed)" << std::endl;
        std::cout << "  Falling back to XSigma profiler only" << std::endl;
    }
    else
    {
        std::cout << "✓ ITT profiler initialized (domain: XSigma)" << std::endl;
    }

    // Start XSigma profiler for JSON export
    profiler_options opts;
    opts.enable_timing_  = true;
    opts.output_format_  = profiler_options::output_format_enum::JSON;

    profiler_session session(opts);
    session.start();

    std::cout << "✓ Profiling started" << std::endl;

    // Profile with both ITT and XSigma
    {
        if (itt_available)
        {
            xsigma::profiler::itt_range_push("itt_workload");
        }
        XSIGMA_PROFILE_SCOPE("itt_workload");

        const size_t matrix_size = 60;
        auto         matrix_a    = generate_matrix(matrix_size, matrix_size);
        auto         matrix_b    = generate_matrix(matrix_size, matrix_size);

        {
            if (itt_available)
            {
                xsigma::profiler::itt_range_push("matrix_computation");
            }
            XSIGMA_PROFILE_SCOPE("matrix_computation");

            auto result = matrix_multiply(matrix_a, matrix_b);

            if (itt_available)
            {
                xsigma::profiler::itt_range_pop();
            }
        }

        std::cout << "  Workload completed" << std::endl;

        if (itt_available)
        {
            xsigma::profiler::itt_range_pop();
        }
    }

    // Stop profiling
    session.stop();

    // Export XSigma trace
    std::string const output_file = "itt_xsigma_trace.json";
    session.write_chrome_trace(output_file);

    std::cout << "✓ Profiling stopped" << std::endl;
    std::cout << "✓ XSigma trace saved to: " << output_file << std::endl;

    if (itt_available)
    {
        std::cout << "\nVTune Integration:" << std::endl;
        std::cout << "  Run with VTune: vtune -collect hotspots -app ./example_profiling_basic"
                  << std::endl;
        std::cout << "  View results: vtune-gui" << std::endl;
    }
}

#endif  // XSIGMA_HAS_ITT

}  // namespace xsigma::examples::profiling

// ============================================================================
// Main Function
// ============================================================================

int main()
{
    std::cout << "============================================" << std::endl;
    std::cout << "XSigma Profiling Examples" << std::endl;
    std::cout << "============================================" << std::endl;

    // Example 1: XSigma Native Profiler
    xsigma::examples::profiling::example_xsigma_native_profiler();

#if XSIGMA_HAS_KINETO
    // Example 2: Kineto Profiler
    xsigma::examples::profiling::example_kineto_profiler();
#else
    std::cout << "\n=== Example 2: Kineto Profiler ===" << std::endl;
    std::cout << "✗ Kineto not available (XSIGMA_HAS_KINETO=0)" << std::endl;
#endif

#if XSIGMA_HAS_ITT
    // Example 3: ITT Profiler
    xsigma::examples::profiling::example_itt_profiler();
#else
    std::cout << "\n=== Example 3: ITT Profiler ===" << std::endl;
    std::cout << "✗ ITT not available (XSIGMA_HAS_ITT=0)" << std::endl;
#endif

    std::cout << "\n============================================" << std::endl;
    std::cout << "All examples completed!" << std::endl;
    std::cout << "============================================" << std::endl;

    return 0;
}

