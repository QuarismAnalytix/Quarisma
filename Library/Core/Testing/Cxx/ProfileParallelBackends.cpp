#if XSIGMA_HAS_NATIVE_PROFILER && 0
/*
 * XSigma Parallel Backends Performance Profiling
 *
 * This file provides comprehensive performance profiling comparison of the three
 * parallel backends (OpenMP, TBB, Native) using Kineto profiler integration.
 *
 * Build Instructions:
 *   Build 1 (OpenMP):  cmake -DXSIGMA_ENABLE_OPENMP=ON  -DXSIGMA_ENABLE_TBB=OFF ..
 *   Build 2 (TBB):     cmake -DXSIGMA_ENABLE_OPENMP=OFF -DXSIGMA_ENABLE_TBB=ON  ..
 *   Build 3 (Native):  cmake -DXSIGMA_ENABLE_OPENMP=OFF -DXSIGMA_ENABLE_TBB=OFF ..
 *
 * Run Instructions:
 *   ./bin/ProfileParallelBackends
 *
 * Output:
 *   - Console output with performance metrics
 *   - Kineto trace file: <backend>_parallel_trace.json
 *   - Chrome trace viewer: chrome://tracing
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "parallel/parallel.h"
#include "profiler/kineto/kineto_shim.h"
#include "profiler/native/session/profiler.h"

#if XSIGMA_HAS_KINETO
#include <ActivityType.h>
#include <libkineto.h>
#endif

namespace xsigma
{
namespace profiling
{

// ============================================================================
// Configuration and Constants
// ============================================================================

/// Workload sizes for profiling
constexpr int64_t kSmallSize  = 1000;     // 1K elements
constexpr int64_t kMediumSize = 10000;    // 10K elements
constexpr int64_t kLargeSize  = 100000;   // 100K elements
constexpr int64_t kXLargeSize = 1000000;  // 1M elements

/// Grain sizes for parallel_for
constexpr int64_t kSmallGrain  = 100;
constexpr int64_t kMediumGrain = 1000;
constexpr int64_t kLargeGrain  = 10000;

/// Thread counts for scaling tests
const std::vector<int> kThreadCounts = {1, 2, 4, 8};

/// Number of iterations for averaging
constexpr int kNumIterations = 5;

// ============================================================================
// Backend Detection
// ============================================================================

/**
 * @brief Get the name of the current parallel backend
 *
 * @return Backend name string
 */
std::string get_backend_name()
{
#if XSIGMA_HAS_OPENMP
    return "openmp";
#elif XSIGMA_HAS_TBB
    return "tbb";
#else
    return "std";
#endif
}

// ============================================================================
// Timing Utilities
// ============================================================================

/**
 * @brief High-resolution timer for performance measurement
 */
class timer
{
public:
    timer() : start_(std::chrono::high_resolution_clock::now()) {}

    /// Reset timer to current time
    void reset() { start_ = std::chrono::high_resolution_clock::now(); }

    /// Get elapsed time in microseconds
    double elapsed_us() const
    {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::micro>(end - start_).count();
    }

    /// Get elapsed time in milliseconds
    double elapsed_ms() const { return elapsed_us() / 1000.0; }

private:
    std::chrono::high_resolution_clock::time_point start_;
};

// ============================================================================
// Workload Functions
// ============================================================================

/**
 * @brief Simple memory-bound workload (array increment)
 *
 * @param data Vector to increment
 */
void workload_memory_bound(std::vector<double>& data)
{
    XSIGMA_PROFILE_SCOPE("workload_memory_bound");

    parallel_for(
        0,
        static_cast<int64_t>(data.size()),
        kMediumGrain,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] += 1.0;
            }
        });
}

/**
 * @brief Compute-intensive workload (trigonometric operations)
 *
 * @param data Vector to process
 */
void workload_compute_intensive(std::vector<double>& data)
{
    XSIGMA_PROFILE_SCOPE("workload_compute_intensive");

    parallel_for(
        0,
        static_cast<int64_t>(data.size()),
        kMediumGrain,
        [&data](int64_t begin, int64_t end)
        {
            for (int64_t i = begin; i < end; ++i)
            {
                double x = data[i];
                // Compute-intensive operations
                for (int j = 0; j < 100; ++j)
                {
                    x = std::sin(x) * std::cos(x) + std::sqrt(std::abs(x));
                }
                data[i] = x;
            }
        });
}

/**
 * @brief Parallel reduction workload (sum)
 *
 * @param data Vector to sum
 * @return Sum of all elements
 */
double workload_reduction_sum(const std::vector<double>& data)
{
    XSIGMA_PROFILE_SCOPE("workload_reduction_sum");

    return parallel_reduce(
        0,
        static_cast<int64_t>(data.size()),
        kMediumGrain,
        0.0,
        [&data](int64_t begin, int64_t end, double init)
        {
            double sum = init;
            for (int64_t i = begin; i < end; ++i)
            {
                sum += data[i];
            }
            return sum;
        },
        [](double a, double b) { return a + b; });
}

/**
 * @brief Parallel reduction workload (max)
 *
 * @param data Vector to find max
 * @return Maximum element
 */
double workload_reduction_max(const std::vector<double>& data)
{
    XSIGMA_PROFILE_SCOPE("workload_reduction_max");

    return parallel_reduce(
        0,
        static_cast<int64_t>(data.size()),
        kMediumGrain,
        -std::numeric_limits<double>::infinity(),
        [&data](int64_t begin, int64_t end, double init)
        {
            double max_val = init;
            for (int64_t i = begin; i < end; ++i)
            {
                max_val = std::max(max_val, data[i]);
            }
            return max_val;
        },
        [](double a, double b) { return std::max(a, b); });
}

/**
 * @brief Parallel reduction workload (min)
 *
 * @param data Vector to find min
 * @return Minimum element
 */
double workload_reduction_min(const std::vector<double>& data)
{
    XSIGMA_PROFILE_SCOPE("workload_reduction_min");

    return parallel_reduce(
        0,
        static_cast<int64_t>(data.size()),
        kMediumGrain,
        std::numeric_limits<double>::infinity(),
        [&data](int64_t begin, int64_t end, double init)
        {
            double min_val = init;
            for (int64_t i = begin; i < end; ++i)
            {
                min_val = std::min(min_val, data[i]);
            }
            return min_val;
        },
        [](double a, double b) { return std::min(a, b); });
}

// ============================================================================
// Profiling Test Functions
// ============================================================================

/**
 * @brief Profile parallel_for with varying grain sizes
 */
void profile_grain_size_variation()
{
    XSIGMA_PROFILE_SCOPE("profile_grain_size_variation");

    std::cout << "\n=== Grain Size Variation ===" << std::endl;

    std::vector<double>        data(kMediumSize, 1.0);
    const std::vector<int64_t> grain_sizes = {kSmallGrain, kMediumGrain, kLargeGrain};

    for (int64_t grain : grain_sizes)
    {
        XSIGMA_PROFILE_SCOPE("grain_size_test");

        timer t;
        parallel_for(
            0,
            static_cast<int64_t>(data.size()),
            grain,
            [&data](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    data[i] += 1.0;
                }
            });

        double elapsed = t.elapsed_ms();
        std::cout << "  Grain size " << std::setw(6) << grain << ": " << std::fixed
                  << std::setprecision(3) << elapsed << " ms" << std::endl;
    }
}

/**
 * @brief Profile thread scaling (1, 2, 4, 8 threads)
 */
void profile_thread_scaling()
{
    XSIGMA_PROFILE_SCOPE("profile_thread_scaling");

    std::cout << "\n=== Thread Scaling ===" << std::endl;

    std::vector<double> data(kLargeSize, 1.0);

    for (int num_threads : kThreadCounts)
    {
        XSIGMA_PROFILE_SCOPE("thread_scaling_test");

        set_num_threads(num_threads);

        timer t;
        workload_compute_intensive(data);
        double elapsed = t.elapsed_ms();

        std::cout << "  Threads " << num_threads << ": " << std::fixed << std::setprecision(3)
                  << elapsed << " ms" << std::endl;
    }
}

/**
 * @brief Profile different data sizes
 */
void profile_data_size_variation()
{
    XSIGMA_PROFILE_SCOPE("profile_data_size_variation");

    std::cout << "\n=== Data Size Variation ===" << std::endl;

    const std::vector<int64_t>     sizes      = {kSmallSize, kMediumSize, kLargeSize, kXLargeSize};
    const std::vector<std::string> size_names = {"1K", "10K", "100K", "1M"};

    for (size_t i = 0; i < sizes.size(); ++i)
    {
        XSIGMA_PROFILE_SCOPE("data_size_test");

        std::vector<double> data(sizes[i], 1.0);

        timer t;
        workload_memory_bound(data);
        double elapsed = t.elapsed_ms();

        std::cout << "  Size " << std::setw(4) << size_names[i] << ": " << std::fixed
                  << std::setprecision(3) << elapsed << " ms" << std::endl;
    }
}

/**
 * @brief Profile reduction operations
 */
void profile_reduction_operations()
{
    XSIGMA_PROFILE_SCOPE("profile_reduction_operations");

    std::cout << "\n=== Reduction Operations ===" << std::endl;

    std::vector<double> data(kLargeSize);
    for (size_t i = 0; i < data.size(); ++i)
    {
        data[i] = static_cast<double>(i);
    }

    // Sum reduction
    {
        XSIGMA_PROFILE_SCOPE("reduction_sum");
        timer  t;
        double sum     = workload_reduction_sum(data);
        double elapsed = t.elapsed_ms();
        std::cout << "  Sum reduction: " << std::fixed << std::setprecision(3) << elapsed
                  << " ms (result: " << sum << ")" << std::endl;
    }

    // Max reduction
    {
        XSIGMA_PROFILE_SCOPE("reduction_max");
        timer  t;
        double max_val = workload_reduction_max(data);
        double elapsed = t.elapsed_ms();
        std::cout << "  Max reduction: " << std::fixed << std::setprecision(3) << elapsed
                  << " ms (result: " << max_val << ")" << std::endl;
    }

    // Min reduction
    {
        XSIGMA_PROFILE_SCOPE("reduction_min");
        timer  t;
        double min_val = workload_reduction_min(data);
        double elapsed = t.elapsed_ms();
        std::cout << "  Min reduction: " << std::fixed << std::setprecision(3) << elapsed
                  << " ms (result: " << min_val << ")" << std::endl;
    }
}

/**
 * @brief Profile workload types (memory-bound vs compute-intensive)
 */
void profile_workload_types()
{
    XSIGMA_PROFILE_SCOPE("profile_workload_types");

    std::cout << "\n=== Workload Types ===" << std::endl;

    std::vector<double> data(kMediumSize, 1.0);

    // Memory-bound workload
    {
        XSIGMA_PROFILE_SCOPE("memory_bound_workload");
        timer t;
        workload_memory_bound(data);
        double elapsed = t.elapsed_ms();
        std::cout << "  Memory-bound: " << std::fixed << std::setprecision(3) << elapsed << " ms"
                  << std::endl;
    }

    // Compute-intensive workload
    {
        XSIGMA_PROFILE_SCOPE("compute_intensive_workload");
        timer t;
        workload_compute_intensive(data);
        double elapsed = t.elapsed_ms();
        std::cout << "  Compute-intensive: " << std::fixed << std::setprecision(3) << elapsed
                  << " ms" << std::endl;
    }
}

/**
 * @brief Profile parallel region overhead
 */
void profile_parallel_overhead()
{
    XSIGMA_PROFILE_SCOPE("profile_parallel_overhead");

    std::cout << "\n=== Parallel Region Overhead ===" << std::endl;

    const int           num_iterations = 1000;
    std::vector<double> data(100, 1.0);

    timer t;
    for (int i = 0; i < num_iterations; ++i)
    {
        parallel_for(
            0,
            static_cast<int64_t>(data.size()),
            10,
            [&data](int64_t begin, int64_t end)
            {
                for (int64_t j = begin; j < end; ++j)
                {
                    data[j] += 1.0;
                }
            });
    }
    double elapsed           = t.elapsed_ms();
    double overhead_per_call = elapsed / num_iterations;

    std::cout << "  Total time for " << num_iterations << " parallel regions: " << std::fixed
              << std::setprecision(3) << elapsed << " ms" << std::endl;
    std::cout << "  Average overhead per parallel region: " << std::fixed << std::setprecision(6)
              << overhead_per_call << " ms" << std::endl;
}

/**
 * @brief Comprehensive profiling suite
 */
void run_comprehensive_profiling()
{
    XSIGMA_PROFILE_SCOPE("run_comprehensive_profiling");

    std::string backend = get_backend_name();

    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "XSigma Parallel Backend Performance Profiling" << std::endl;
    std::cout << "Backend: " << backend << std::endl;
    std::cout << "Threads: " << get_num_threads() << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    // Run all profiling tests
    profile_grain_size_variation();
    profile_thread_scaling();
    profile_data_size_variation();
    profile_reduction_operations();
    profile_workload_types();
    profile_parallel_overhead();

    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "Profiling Complete" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

}  // namespace profiling
}  // namespace xsigma

// ============================================================================
// Main Function
// ============================================================================

int main(int argc, char** argv)
{
    using namespace xsigma;
    using namespace xsigma::profiling;

    std::cout << "XSigma Parallel Backends Performance Profiling" << std::endl;
    std::cout << "Backend: " << get_backend_name() << std::endl;
    std::cout << "Parallel Info:\n" << get_parallel_info() << std::endl;

#if XSIGMA_HAS_KINETO
    // Initialize Kineto profiler
    std::cout << "\nInitializing Kineto profiler..." << std::endl;
    profiler::kineto_init(true, true);  // CPU only, log on error

    if (!profiler::kineto_is_profiler_registered())
    {
        std::cerr << "Warning: Kineto profiler not available - running without profiling"
                  << std::endl;
        run_comprehensive_profiling();
        return 0;
    }

    std::cout << "Kineto profiler initialized successfully" << std::endl;

    // Prepare Kineto trace
    std::set<libkineto::ActivityType> activities;
    activities.insert(libkineto::ActivityType::CPU_OP);

    profiler::kineto_prepare_trace(activities);
    profiler::kineto_start_trace();
    std::cout << "Kineto trace started" << std::endl;
#endif

    // Start XSigma profiler for hierarchical CPU profiling
    profiler_options opts;
    opts.enable_timing_ = true;
    opts.output_format_ = profiler_options::output_format_enum::JSON;

    profiler_session session(opts);
    session.start();
    std::cout << "XSigma profiler started" << std::endl;

    // Run comprehensive profiling
    run_comprehensive_profiling();

    // Stop XSigma profiler
    session.stop();

    // Generate output filename based on backend
    std::string backend_name = get_backend_name();
    std::transform(backend_name.begin(), backend_name.end(), backend_name.begin(), ::tolower);
    std::string xsigma_trace_file = backend_name + "_xsigma_trace.json";

    session.write_chrome_trace(xsigma_trace_file);
    std::cout << "\nXSigma trace saved to: " << xsigma_trace_file << std::endl;

#if XSIGMA_HAS_KINETO
    // Stop Kineto trace
    std::unique_ptr<libkineto::ActivityTraceInterface> kineto_trace(
        static_cast<libkineto::ActivityTraceInterface*>(profiler::kineto_stop_trace()));

    if (kineto_trace)
    {
        std::string kineto_trace_file = backend_name + "_kineto_trace.json";
        kineto_trace->save(kineto_trace_file);
        std::cout << "Kineto trace saved to: " << kineto_trace_file << std::endl;
    }
#endif

    std::cout << "\nProfiling complete!" << std::endl;
    std::cout << "View traces in Chrome: chrome://tracing" << std::endl;

    return 0;
}
#else
int main(int argc, char** argv)
{
    return 0;
}
#endif  // XSIGMA_HAS_NATIVE_PROFILER
