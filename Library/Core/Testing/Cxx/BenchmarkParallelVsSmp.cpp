/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Comprehensive Performance Benchmark: parallel module vs smp module
 *
 * This benchmark suite compares the performance characteristics of the
 * Library/Core/parallel/ module against the Library/Core/smp/ module across
 * all available backends and workload types.
 *
 * Benchmark Coverage:
 * - Parallel for loops (index-based iteration)
 * - Parallel reductions (sum, max, min operations)
 * - Work distribution and load balancing
 * - Scalability with varying thread counts
 * - Small, medium, and large workloads
 * - Compute-intensive vs memory-intensive operations
 *
 * Backend Combinations:
 * - parallel module: OpenMP backend (if XSIGMA_HAS_OPENMP=1), Native backend
 * - smp module: std_thread backend, TBB backend (if XSIGMA_HAS_TBB=1)
 *
 * Metrics Measured:
 * - Execution time (wall clock time)
 * - Throughput (elements processed per second)
 * - Speedup vs sequential baseline
 * - Parallel efficiency
 *
 * Coding Standards:
 * - Follows XSigma C++ coding standards (snake_case, no exceptions)
 * - Uses Google Benchmark library for accurate measurements
 * - Ensures fair comparison: identical workloads, same compiler optimizations
 * - Multiple iterations for stable results
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

#include "parallel/parallel.h"
#include "smp/smp_tools.h"

namespace xsigma
{

// =============================================================================
// Benchmark Configuration Constants
// =============================================================================

// Workload sizes
constexpr int64_t kSmallSize  = 1000;
constexpr int64_t kMediumSize = 100000;
constexpr int64_t kLargeSize  = 10000000;

// Grain sizes (minimum chunk size for parallel work distribution)
constexpr int64_t kSmallGrain  = 100;
constexpr int64_t kMediumGrain = 1000;
constexpr int64_t kLargeGrain  = 10000;

// =============================================================================
// sequential Baseline Benchmarks
// =============================================================================

/**
 * @brief sequential baseline for simple memory operations
 *
 * Provides baseline performance for comparison with parallel implementations.
 * Measures simple array assignment without parallelization overhead.
 */
static void BM_Sequential_Simple_Small(benchmark::State& state)
{
    std::vector<int> data(kSmallSize, 0);

    for (auto _ : state)
    {
        for (int64_t i = 0; i < kSmallSize; ++i)
        {
            data[i] = static_cast<int>(i * 2);
        }
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * kSmallSize);
    state.SetLabel("sequential-Simple-Small");
}
BENCHMARK(BM_Sequential_Simple_Small);

static void BM_Sequential_Simple_Medium(benchmark::State& state)
{
    std::vector<int> data(kMediumSize, 0);

    for (auto _ : state)
    {
        for (int64_t i = 0; i < kMediumSize; ++i)
        {
            data[i] = static_cast<int>(i * 2);
        }
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * kMediumSize);
    state.SetLabel("sequential-Simple-Medium");
}
BENCHMARK(BM_Sequential_Simple_Medium);

static void BM_Sequential_Simple_Large(benchmark::State& state)
{
    std::vector<int> data(kLargeSize, 0);

    for (auto _ : state)
    {
        for (int64_t i = 0; i < kLargeSize; ++i)
        {
            data[i] = static_cast<int>(i * 2);
        }
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * kLargeSize);
    state.SetLabel("sequential-Simple-Large");
}
BENCHMARK(BM_Sequential_Simple_Large);

/**
 * @brief sequential baseline for compute-intensive operations
 *
 * Measures performance of polynomial computation without parallelization.
 * Used to calculate speedup and parallel efficiency.
 */
static void BM_Sequential_Compute_Medium(benchmark::State& state)
{
    std::vector<double> data(kMediumSize, 0.0);

    for (auto _ : state)
    {
        for (int64_t i = 0; i < kMediumSize; ++i)
        {
            double x = static_cast<double>(i);
            data[i]  = x * x * x + 3.0 * x * x + 3.0 * x + 1.0;
        }
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * kMediumSize);
    state.SetLabel("sequential-Compute-Medium");
}
BENCHMARK(BM_Sequential_Compute_Medium);

/**
 * @brief sequential baseline for reduction operations
 *
 * Measures performance of sum reduction without parallelization.
 */
static void BM_Sequential_Reduce_Sum_Medium(benchmark::State& state)
{
    for (auto _ : state)
    {
        int64_t sum = 0;
        for (int64_t i = 0; i < kMediumSize; ++i)
        {
            sum += i;
        }
        benchmark::DoNotOptimize(sum);
    }

    state.SetItemsProcessed(state.iterations() * kMediumSize);
    state.SetLabel("sequential-Reduce-Sum-Medium");
}
BENCHMARK(BM_Sequential_Reduce_Sum_Medium);

// =============================================================================
// Parallel Module Benchmarks - Simple Memory Operations
// =============================================================================

/**
 * @brief parallel_for with small workload
 *
 * Tests parallel module performance on small datasets where parallelization
 * overhead may dominate actual computation time.
 */
static void BM_Parallel_For_Simple_Small(benchmark::State& state)
{
    std::vector<int> data(kSmallSize, 0);

    for (auto _ : state)
    {
        parallel_for(
            0,
            kSmallSize,
            kSmallGrain,
            [&data](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    data[i] = static_cast<int>(i * 2);
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * kSmallSize);
    state.SetLabel("Parallel-For-Simple-Small");
}
BENCHMARK(BM_Parallel_For_Simple_Small);

static void BM_Parallel_For_Simple_Medium(benchmark::State& state)
{
    std::vector<int> data(kMediumSize, 0);

    for (auto _ : state)
    {
        parallel_for(
            0,
            kMediumSize,
            kMediumGrain,
            [&data](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    data[i] = static_cast<int>(i * 2);
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * kMediumSize);
    state.SetLabel("Parallel-For-Simple-Medium");
}
BENCHMARK(BM_Parallel_For_Simple_Medium);

static void BM_Parallel_For_Simple_Large(benchmark::State& state)
{
    std::vector<int> data(kLargeSize, 0);

    for (auto _ : state)
    {
        parallel_for(
            0,
            kLargeSize,
            kLargeGrain,
            [&data](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    data[i] = static_cast<int>(i * 2);
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * kLargeSize);
    state.SetLabel("Parallel-For-Simple-Large");
}
BENCHMARK(BM_Parallel_For_Simple_Large);

// =============================================================================
// SMP Module Benchmarks - Simple Memory Operations
// =============================================================================

/**
 * @brief smp tools::For with small workload
 *
 * Tests smp module performance on small datasets for direct comparison
 * with parallel module.
 */
static void BM_SMP_For_Simple_Small(benchmark::State& state)
{
    std::vector<int> data(kSmallSize, 0);

    for (auto _ : state)
    {
        smp_tools::parallel_for(
            0,
            static_cast<int>(kSmallSize),
            static_cast<int>(kSmallGrain),
            [&data](int begin, int end)
            {
                for (int i = begin; i < end; ++i)
                {
                    data[i] = i * 2;
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * kSmallSize);
    state.SetLabel("SMP-For-Simple-Small");
}
BENCHMARK(BM_SMP_For_Simple_Small);

static void BM_SMP_For_Simple_Medium(benchmark::State& state)
{
    std::vector<int> data(kMediumSize, 0);

    for (auto _ : state)
    {
        smp_tools::parallel_for(
            0,
            static_cast<int>(kMediumSize),
            static_cast<int>(kMediumGrain),
            [&data](int begin, int end)
            {
                for (int i = begin; i < end; ++i)
                {
                    data[i] = i * 2;
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * kMediumSize);
    state.SetLabel("SMP-For-Simple-Medium");
}
BENCHMARK(BM_SMP_For_Simple_Medium);

static void BM_SMP_For_Simple_Large(benchmark::State& state)
{
    std::vector<int> data(kLargeSize, 0);

    for (auto _ : state)
    {
        smp_tools::parallel_for(
            0,
            static_cast<int>(kLargeSize),
            static_cast<int>(kLargeGrain),
            [&data](int begin, int end)
            {
                for (int i = begin; i < end; ++i)
                {
                    data[i] = i * 2;
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * kLargeSize);
    state.SetLabel("SMP-For-Simple-Large");
}
BENCHMARK(BM_SMP_For_Simple_Large);

// =============================================================================
// Parallel Module Benchmarks - Compute-Intensive Operations
// =============================================================================

/**
 * @brief parallel_for with compute-intensive workload
 *
 * Tests parallel module performance on compute-bound operations where
 * parallelization should provide significant speedup.
 */
static void BM_Parallel_For_Compute_Small(benchmark::State& state)
{
    std::vector<double> data(kSmallSize, 0.0);

    for (auto _ : state)
    {
        parallel_for(
            0,
            kSmallSize,
            kSmallGrain,
            [&data](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    double x = static_cast<double>(i);
                    data[i]  = x * x * x + 3.0 * x * x + 3.0 * x + 1.0;
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * kSmallSize);
    state.SetLabel("Parallel-For-Compute-Small");
}
BENCHMARK(BM_Parallel_For_Compute_Small);

static void BM_Parallel_For_Compute_Medium(benchmark::State& state)
{
    std::vector<double> data(kMediumSize, 0.0);

    for (auto _ : state)
    {
        parallel_for(
            0,
            kMediumSize,
            kMediumGrain,
            [&data](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    double x = static_cast<double>(i);
                    data[i]  = x * x * x + 3.0 * x * x + 3.0 * x + 1.0;
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * kMediumSize);
    state.SetLabel("Parallel-For-Compute-Medium");
}
BENCHMARK(BM_Parallel_For_Compute_Medium);

static void BM_Parallel_For_Compute_Large(benchmark::State& state)
{
    std::vector<double> data(kLargeSize, 0.0);

    for (auto _ : state)
    {
        parallel_for(
            0,
            kLargeSize,
            kLargeGrain,
            [&data](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    double x = static_cast<double>(i);
                    data[i]  = x * x * x + 3.0 * x * x + 3.0 * x + 1.0;
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * kLargeSize);
    state.SetLabel("Parallel-For-Compute-Large");
}
BENCHMARK(BM_Parallel_For_Compute_Large);

// =============================================================================
// SMP Module Benchmarks - Compute-Intensive Operations
// =============================================================================

/**
 * @brief smp tools::For with compute-intensive workload
 *
 * Tests smp module performance on compute-bound operations for direct
 * comparison with parallel module.
 */
static void BM_SMP_For_Compute_Small(benchmark::State& state)
{
    std::vector<double> data(kSmallSize, 0.0);

    for (auto _ : state)
    {
        smp_tools::parallel_for(
            0,
            static_cast<int>(kSmallSize),
            static_cast<int>(kSmallGrain),
            [&data](int begin, int end)
            {
                for (int i = begin; i < end; ++i)
                {
                    double x = static_cast<double>(i);
                    data[i]  = x * x * x + 3.0 * x * x + 3.0 * x + 1.0;
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * kSmallSize);
    state.SetLabel("SMP-For-Compute-Small");
}
BENCHMARK(BM_SMP_For_Compute_Small);

static void BM_SMP_For_Compute_Medium(benchmark::State& state)
{
    std::vector<double> data(kMediumSize, 0.0);

    for (auto _ : state)
    {
        smp_tools::parallel_for(
            0,
            static_cast<int>(kMediumSize),
            static_cast<int>(kMediumGrain),
            [&data](int begin, int end)
            {
                for (int i = begin; i < end; ++i)
                {
                    double x = static_cast<double>(i);
                    data[i]  = x * x * x + 3.0 * x * x + 3.0 * x + 1.0;
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * kMediumSize);
    state.SetLabel("SMP-For-Compute-Medium");
}
BENCHMARK(BM_SMP_For_Compute_Medium);

static void BM_SMP_For_Compute_Large(benchmark::State& state)
{
    std::vector<double> data(kLargeSize, 0.0);

    for (auto _ : state)
    {
        smp_tools::parallel_for(
            0,
            static_cast<int>(kLargeSize),
            static_cast<int>(kLargeGrain),
            [&data](int begin, int end)
            {
                for (int i = begin; i < end; ++i)
                {
                    double x = static_cast<double>(i);
                    data[i]  = x * x * x + 3.0 * x * x + 3.0 * x + 1.0;
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * kLargeSize);
    state.SetLabel("SMP-For-Compute-Large");
}
BENCHMARK(BM_SMP_For_Compute_Large);

// =============================================================================
// Parallel Module Benchmarks - Reduction Operations
// =============================================================================

/**
 * @brief parallel_reduce for sum operation
 *
 * Tests parallel module's reduction performance. Reductions are critical
 * operations that require efficient combining of partial results.
 */
static void BM_Parallel_Reduce_Sum_Small(benchmark::State& state)
{
    for (auto _ : state)
    {
        int64_t result = parallel_reduce(
            0,
            kSmallSize,
            kSmallGrain,
            static_cast<int64_t>(0),
            [](int64_t begin, int64_t end, int64_t identity)
            {
                int64_t sum = identity;
                for (int64_t i = begin; i < end; ++i)
                {
                    sum += i;
                }
                return sum;
            },
            [](int64_t a, int64_t b) { return a + b; });
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * kSmallSize);
    state.SetLabel("Parallel-Reduce-Sum-Small");
}
BENCHMARK(BM_Parallel_Reduce_Sum_Small);

static void BM_Parallel_Reduce_Sum_Medium(benchmark::State& state)
{
    for (auto _ : state)
    {
        int64_t result = parallel_reduce(
            0,
            kMediumSize,
            kMediumGrain,
            static_cast<int64_t>(0),
            [](int64_t begin, int64_t end, int64_t identity)
            {
                int64_t sum = identity;
                for (int64_t i = begin; i < end; ++i)
                {
                    sum += i;
                }
                return sum;
            },
            [](int64_t a, int64_t b) { return a + b; });
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * kMediumSize);
    state.SetLabel("Parallel-Reduce-Sum-Medium");
}
BENCHMARK(BM_Parallel_Reduce_Sum_Medium);

static void BM_Parallel_Reduce_Sum_Large(benchmark::State& state)
{
    for (auto _ : state)
    {
        int64_t result = parallel_reduce(
            0,
            kLargeSize,
            kLargeGrain,
            static_cast<int64_t>(0),
            [](int64_t begin, int64_t end, int64_t identity)
            {
                int64_t sum = identity;
                for (int64_t i = begin; i < end; ++i)
                {
                    sum += i;
                }
                return sum;
            },
            [](int64_t a, int64_t b) { return a + b; });
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * kLargeSize);
    state.SetLabel("Parallel-Reduce-Sum-Large");
}
BENCHMARK(BM_Parallel_Reduce_Sum_Large);

/**
 * @brief parallel_reduce for max operation
 *
 * Tests reduction with non-commutative operation (max).
 */
static void BM_Parallel_Reduce_Max_Medium(benchmark::State& state)
{
    for (auto _ : state)
    {
        int64_t result = parallel_reduce(
            0,
            kMediumSize,
            kMediumGrain,
            static_cast<int64_t>(0),
            [](int64_t begin, int64_t end, int64_t identity)
            {
                int64_t max_val = identity;
                for (int64_t i = begin; i < end; ++i)
                {
                    max_val = std::max(max_val, i);
                }
                return max_val;
            },
            [](int64_t a, int64_t b) { return std::max(a, b); });
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * kMediumSize);
    state.SetLabel("Parallel-Reduce-Max-Medium");
}
BENCHMARK(BM_Parallel_Reduce_Max_Medium);

/**
 * @brief parallel_reduce for compute-intensive reduction
 *
 * Tests reduction with expensive per-element computation.
 */
static void BM_Parallel_Reduce_Compute_Medium(benchmark::State& state)
{
    for (auto _ : state)
    {
        double result = parallel_reduce(
            0,
            kMediumSize,
            kMediumGrain,
            0.0,
            [](int64_t begin, int64_t end, double identity)
            {
                double sum = identity;
                for (int64_t i = begin; i < end; ++i)
                {
                    double x = static_cast<double>(i);
                    sum += std::sqrt(x * x + 1.0);
                }
                return sum;
            },
            [](double a, double b) { return a + b; });
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * kMediumSize);
    state.SetLabel("Parallel-Reduce-Compute-Medium");
}
BENCHMARK(BM_Parallel_Reduce_Compute_Medium);

// =============================================================================
// Memory-Intensive Benchmarks
// =============================================================================

/**
 * @brief parallel_for with memory-intensive workload
 *
 * Tests performance when memory bandwidth is the bottleneck rather than
 * computation. Uses large data structures to stress memory subsystem.
 */
static void BM_Parallel_For_Memory_Medium(benchmark::State& state)
{
    std::vector<double> input(kMediumSize);
    std::vector<double> output(kMediumSize);

    // Initialize input data
    for (int64_t i = 0; i < kMediumSize; ++i)
    {
        input[i] = static_cast<double>(i);
    }

    for (auto _ : state)
    {
        parallel_for(
            0,
            kMediumSize,
            kMediumGrain,
            [&input, &output](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    // Simple memory copy with minimal computation
                    output[i] = input[i] * 1.01;
                }
            });
        benchmark::DoNotOptimize(output.data());
    }

    state.SetItemsProcessed(state.iterations() * kMediumSize);
    state.SetBytesProcessed(state.iterations() * kMediumSize * 2 * sizeof(double));
    state.SetLabel("Parallel-For-Memory-Medium");
}
BENCHMARK(BM_Parallel_For_Memory_Medium);

static void BM_SMP_For_Memory_Medium(benchmark::State& state)
{
    std::vector<double> input(kMediumSize);
    std::vector<double> output(kMediumSize);

    // Initialize input data
    for (int64_t i = 0; i < kMediumSize; ++i)
    {
        input[i] = static_cast<double>(i);
    }

    for (auto _ : state)
    {
        smp_tools::parallel_for(
            0,
            static_cast<int>(kMediumSize),
            static_cast<int>(kMediumGrain),
            [&input, &output](int begin, int end)
            {
                for (int i = begin; i < end; ++i)
                {
                    // Simple memory copy with minimal computation
                    output[i] = input[i] * 1.01;
                }
            });
        benchmark::DoNotOptimize(output.data());
    }

    state.SetItemsProcessed(state.iterations() * kMediumSize);
    state.SetBytesProcessed(state.iterations() * kMediumSize * 2 * sizeof(double));
    state.SetLabel("SMP-For-Memory-Medium");
}
BENCHMARK(BM_SMP_For_Memory_Medium);

// =============================================================================
// Thread Scaling Benchmarks
// =============================================================================

/**
 * @brief Benchmark parallel_for with varying thread counts
 *
 * Measures scalability by running the same workload with different numbers
 * of threads. Helps identify parallel efficiency and overhead.
 */
static void BM_Parallel_For_ThreadScaling(benchmark::State& state)
{
    const int           num_threads = static_cast<int>(state.range(0));
    std::vector<double> data(kMediumSize, 0.0);

    // Set thread count for this benchmark
    const int original_threads = get_num_threads();
    set_num_threads(num_threads);

    for (auto _ : state)
    {
        parallel_for(
            0,
            kMediumSize,
            kMediumGrain,
            [&data](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    double x = static_cast<double>(i);
                    data[i]  = x * x * x + 3.0 * x * x + 3.0 * x + 1.0;
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    // Restore original thread count
    set_num_threads(original_threads);

    state.SetItemsProcessed(state.iterations() * kMediumSize);
    state.SetLabel("Parallel-Threads-" + std::to_string(num_threads));
}
BENCHMARK(BM_Parallel_For_ThreadScaling)->DenseRange(1, 8, 1);

static void BM_SMP_For_ThreadScaling(benchmark::State& state)
{
    const int           num_threads = static_cast<int>(state.range(0));
    std::vector<double> data(kMediumSize, 0.0);

    // Set thread count for this benchmark
    smp_tools::initialize(num_threads);

    for (auto _ : state)
    {
        smp_tools::parallel_for(
            0,
            static_cast<int>(kMediumSize),
            static_cast<int>(kMediumGrain),
            [&data](int begin, int end)
            {
                for (int i = begin; i < end; ++i)
                {
                    double x = static_cast<double>(i);
                    data[i]  = x * x * x + 3.0 * x * x + 3.0 * x + 1.0;
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * kMediumSize);
    state.SetLabel("SMP-Threads-" + std::to_string(num_threads));
}
BENCHMARK(BM_SMP_For_ThreadScaling)->DenseRange(1, 8, 1);

// =============================================================================
// Grain Size Variation Benchmarks
// =============================================================================

/**
 * @brief Benchmark parallel_for with varying grain sizes
 *
 * Tests how grain size (chunk size) affects performance. Smaller grain sizes
 * increase parallelism but also increase overhead. Larger grain sizes reduce
 * overhead but may cause load imbalance.
 */
static void BM_Parallel_For_GrainSize(benchmark::State& state)
{
    const int64_t    grain_size = state.range(0);
    std::vector<int> data(kMediumSize, 0);

    for (auto _ : state)
    {
        parallel_for(
            0,
            kMediumSize,
            grain_size,
            [&data](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    data[i] = static_cast<int>(i * 2);
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * kMediumSize);
    state.SetLabel("Parallel-Grain-" + std::to_string(grain_size));
}
BENCHMARK(BM_Parallel_For_GrainSize)->Arg(100)->Arg(500)->Arg(1000)->Arg(5000)->Arg(10000);

static void BM_SMP_For_GrainSize(benchmark::State& state)
{
    const int        grain_size = static_cast<int>(state.range(0));
    std::vector<int> data(kMediumSize, 0);

    for (auto _ : state)
    {
        smp_tools::parallel_for(
            0,
            static_cast<int>(kMediumSize),
            grain_size,
            [&data](int begin, int end)
            {
                for (int i = begin; i < end; ++i)
                {
                    data[i] = i * 2;
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * kMediumSize);
    state.SetLabel("SMP-Grain-" + std::to_string(grain_size));
}
BENCHMARK(BM_SMP_For_GrainSize)->Arg(100)->Arg(500)->Arg(1000)->Arg(5000)->Arg(10000);

// =============================================================================
// Nested Parallelism Benchmarks
// =============================================================================

/**
 * @brief Benchmark nested parallel_for calls
 *
 * Tests performance when parallel regions are nested. This is important for
 * understanding how the modules handle recursive parallelism.
 */
static void BM_Parallel_For_Nested(benchmark::State& state)
{
    const int64_t                 outer_size = 100;
    const int64_t                 inner_size = 1000;
    std::vector<std::vector<int>> data(outer_size, std::vector<int>(inner_size, 0));

    for (auto _ : state)
    {
        parallel_for(
            0,
            outer_size,
            10,
            [&data, inner_size](int64_t outer_begin, int64_t outer_end)
            {
                for (int64_t i = outer_begin; i < outer_end; ++i)
                {
                    // Inner sequential loop (nested parallelism disabled by default)
                    for (int64_t j = 0; j < inner_size; ++j)
                    {
                        data[i][j] = static_cast<int>(i * inner_size + j);
                    }
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * outer_size * inner_size);
    state.SetLabel("Parallel-Nested");
}
BENCHMARK(BM_Parallel_For_Nested);

static void BM_SMP_For_Nested(benchmark::State& state)
{
    const int                     outer_size = 100;
    const int                     inner_size = 1000;
    std::vector<std::vector<int>> data(outer_size, std::vector<int>(inner_size, 0));

    for (auto _ : state)
    {
        smp_tools::parallel_for(
            0,
            outer_size,
            10,
            [&data, inner_size](int outer_begin, int outer_end)
            {
                for (int i = outer_begin; i < outer_end; ++i)
                {
                    // Inner sequential loop
                    for (int j = 0; j < inner_size; ++j)
                    {
                        data[i][j] = i * inner_size + j;
                    }
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * outer_size * inner_size);
    state.SetLabel("SMP-Nested");
}
BENCHMARK(BM_SMP_For_Nested);

// =============================================================================
// Overhead Measurement Benchmarks
// =============================================================================

/**
 * @brief Benchmark parallel_for with minimal work per element
 *
 * Measures pure parallelization overhead by doing almost no work per element.
 * This helps quantify the fixed cost of parallelization.
 */
static void BM_Parallel_For_Overhead(benchmark::State& state)
{
    std::vector<int> data(kMediumSize, 0);

    for (auto _ : state)
    {
        parallel_for(
            0,
            kMediumSize,
            kMediumGrain,
            [&data](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    // Minimal work - just assignment
                    data[i] = 1;
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * kMediumSize);
    state.SetLabel("Parallel-Overhead");
}
BENCHMARK(BM_Parallel_For_Overhead);

static void BM_SMP_For_Overhead(benchmark::State& state)
{
    std::vector<int> data(kMediumSize, 0);

    for (auto _ : state)
    {
        smp_tools::parallel_for(
            0,
            static_cast<int>(kMediumSize),
            static_cast<int>(kMediumGrain),
            [&data](int begin, int end)
            {
                for (int i = begin; i < end; ++i)
                {
                    // Minimal work - just assignment
                    data[i] = 1;
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * kMediumSize);
    state.SetLabel("SMP-Overhead");
}
BENCHMARK(BM_SMP_For_Overhead);
}  // namespace xsigma

// Note: BENCHMARK_MAIN() is not needed - using benchmark_main library instead
