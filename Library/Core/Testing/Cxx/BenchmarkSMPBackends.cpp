/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * This file is part of XSigma and is licensed under a dual-license model:
 *
 *   - Open-source License (GPLv3):
 *       Free for personal, academic, and research use under the terms of
 *       the GNU General Public License v3.0 or later.
 *
 *   - Commercial License:
 *       A commercial license is required for proprietary, closed-source,
 *       or SaaS usage. Contact us to obtain a commercial agreement.
 *
 * Contact: licensing@xsigma.co.uk
 * Website: https://www.xsigma.co.uk
 */

/**
 * @file BenchmarkSMPBackends.cpp
 * @brief Comprehensive benchmarks for all SMP backend implementations
 *
 * This benchmark suite measures performance across:
 * - std_thread backend (native C++ threads)
 * - OpenMP backend (if XSIGMA_HAS_OPENMP)
 * - TBB backend (if XSIGMA_HAS_TBB)
 *
 * Tests include:
 * - Parallel for loops with varying work sizes
 * - Computationally heavy workloads (matrix operations, numerical integration)
 * - Thread scaling (1, 2, 4, 8, 16 threads)
 * - Overhead measurements
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <cmath>
#include <random>
#include <vector>

#include "parallel/parallel.h"
#include "smp/common/smp_tools_api.h"
#include "smp/smp_tools.h"

namespace xsigma
{
namespace benchmark_detail
{

// ============================================================================
// Computationally Heavy Workloads
// ============================================================================

/**
 * @brief Compute-intensive function simulating numerical integration
 * Uses Simpson's rule for numerical integration of f(x) = sin(x) * exp(-x/10)
 */
inline double compute_heavy_numerical_integration(int64_t start, int64_t end)
{
    const double dx   = 0.001;
    double       sum  = 0.0;
    const double base = static_cast<double>(start);

    for (int64_t i = start; i < end; ++i)
    {
        const double x      = base + static_cast<double>(i) * dx;
        const double x_next = x + dx;
        const double f_x    = std::sin(x) * std::exp(-x / 10.0);
        const double f_next = std::sin(x_next) * std::exp(-x_next / 10.0);
        const double f_mid  = std::sin((x + x_next) / 2.0) * std::exp(-(x + x_next) / 20.0);

        // Simpson's rule: (dx/6) * (f(x) + 4*f(mid) + f(x+dx))
        sum += (f_x + 4.0 * f_mid + f_next) * dx / 6.0;
    }

    return sum;
}

/**
 * @brief Matrix multiplication kernel (compute-heavy)
 * Multiplies two small matrices repeatedly to create CPU-bound work
 */
inline double compute_heavy_matrix_mult(int64_t start, int64_t end)
{
    constexpr int N = 16;  // Small matrix size for cache-friendly computation
    double        A[N][N], B[N][N], C[N][N];

    // Initialize matrices
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            A[i][j] = static_cast<double>(i * N + j) * 0.01;
            B[i][j] = static_cast<double>(j * N + i) * 0.01;
            C[i][j] = 0.0;
        }
    }

    double result = 0.0;

    for (int64_t iter = start; iter < end; ++iter)
    {
        // Matrix multiplication C = A * B
        for (int i = 0; i < N; ++i)
        {
            for (int j = 0; j < N; ++j)
            {
                double sum = 0.0;
                for (int k = 0; k < N; ++k)
                {
                    sum += A[i][k] * B[k][j];
                }
                C[i][j] = sum;
            }
        }

        // Accumulate result to prevent compiler optimization
        result += C[N / 2][N / 2];
    }

    return result;
}

/**
 * @brief Prime number checking (irregular workload)
 * Tests CPU-bound work with variable computational cost per iteration
 */
inline int64_t compute_heavy_prime_check(int64_t start, int64_t end)
{
    int64_t prime_count = 0;

    for (int64_t num = start; num < end; ++num)
    {
        if (num < 2)
            continue;

        bool          is_prime = true;
        const int64_t limit    = static_cast<int64_t>(std::sqrt(static_cast<double>(num))) + 1;

        for (int64_t div = 2; div < limit; ++div)
        {
            if (num % div == 0)
            {
                is_prime = false;
                break;
            }
        }

        if (is_prime)
            ++prime_count;
    }

    return prime_count;
}

}  // namespace benchmark_detail

// ============================================================================
// Parallel For Benchmarks - All Backends
// ============================================================================

/**
 * @brief Benchmark parallel_for with numerical integration workload
 * Tests: Parallel efficiency, load balancing, overhead
 */
static void BM_ParallelFor_Integration(benchmark::State& state)
{
    const int64_t work_size   = state.range(0);
    const int64_t grain_size  = state.range(1);
    const int     num_threads = state.range(2);

    // Set thread count for this benchmark
    set_num_threads(num_threads);

    std::atomic<double> result{0.0};

    for (auto _ : state)
    {
        result.store(0.0, std::memory_order_relaxed);

        parallel_for(
            0,
            work_size,
            grain_size,
            [&result](int64_t begin, int64_t end)
            {
                double local_sum =
                    benchmark_detail::compute_heavy_numerical_integration(begin, end);
                // Atomic accumulation
                double current = result.load(std::memory_order_relaxed);
                while (!result.compare_exchange_weak(
                    current,
                    current + local_sum,
                    std::memory_order_release,
                    std::memory_order_relaxed))
                {
                }
            });

        benchmark::DoNotOptimize(result.load(std::memory_order_acquire));
    }

    const double speedup =
        static_cast<double>(num_threads) / (state.iterations() > 0 ? state.iterations() : 1.0);
    state.SetLabel(
        "Threads=" + std::to_string(num_threads) + " Speedup≈" + std::to_string(speedup));
    state.SetItemsProcessed(state.iterations() * work_size);
}

/**
 * @brief Benchmark parallel_for with matrix multiplication workload
 */
static void BM_ParallelFor_MatrixMult(benchmark::State& state)
{
    const int64_t work_size   = state.range(0);
    const int64_t grain_size  = state.range(1);
    const int     num_threads = state.range(2);

    set_num_threads(num_threads);

    std::atomic<double> result{0.0};

    for (auto _ : state)
    {
        result.store(0.0, std::memory_order_relaxed);

        parallel_for(
            0,
            work_size,
            grain_size,
            [&result](int64_t begin, int64_t end)
            {
                double local_result = benchmark_detail::compute_heavy_matrix_mult(begin, end);
                double current      = result.load(std::memory_order_relaxed);
                while (!result.compare_exchange_weak(
                    current,
                    current + local_result,
                    std::memory_order_release,
                    std::memory_order_relaxed))
                {
                }
            });

        benchmark::DoNotOptimize(result.load(std::memory_order_acquire));
    }

    state.SetItemsProcessed(state.iterations() * work_size);
}

/**
 * @brief Benchmark parallel_for with prime checking (irregular workload)
 */
static void BM_ParallelFor_PrimeCheck(benchmark::State& state)
{
    const int64_t work_size   = state.range(0);
    const int64_t grain_size  = state.range(1);
    const int     num_threads = state.range(2);

    set_num_threads(num_threads);

    std::atomic<int64_t> total_primes{0};

    for (auto _ : state)
    {
        total_primes.store(0, std::memory_order_relaxed);

        parallel_for(
            10000,
            10000 + work_size,
            grain_size,
            [&total_primes](int64_t begin, int64_t end)
            {
                int64_t local_count = benchmark_detail::compute_heavy_prime_check(begin, end);
                total_primes.fetch_add(local_count, std::memory_order_release);
            });

        benchmark::DoNotOptimize(total_primes.load(std::memory_order_acquire));
    }

    state.SetItemsProcessed(state.iterations() * work_size);
}

// ============================================================================
// Parallel Reduce Benchmarks
// ============================================================================

/**
 * @brief Benchmark parallel_reduce with sum reduction
 */
static void BM_ParallelReduce_Sum(benchmark::State& state)
{
    const int64_t work_size   = state.range(0);
    const int64_t grain_size  = state.range(1);
    const int     num_threads = state.range(2);

    set_num_threads(num_threads);

    std::vector<double>                    data(work_size);
    std::mt19937_64                        rng(42);
    std::uniform_real_distribution<double> dist(-1.0, 1.0);

    for (auto& val : data)
    {
        val = dist(rng);
    }

    for (auto _ : state)
    {
        double result = parallel_reduce(
            0,
            work_size,
            grain_size,
            0.0,
            [&data](int64_t begin, int64_t end, double init)
            {
                double sum = init;
                for (int64_t i = begin; i < end; ++i)
                {
                    sum += data[i] * data[i];  // Sum of squares
                }
                return sum;
            },
            [](double a, double b) { return a + b; });

        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * work_size);
}

/**
 * @brief Benchmark parallel_reduce with max reduction
 */
static void BM_ParallelReduce_Max(benchmark::State& state)
{
    const int64_t work_size   = state.range(0);
    const int64_t grain_size  = state.range(1);
    const int     num_threads = state.range(2);

    set_num_threads(num_threads);

    std::vector<double>                    data(work_size);
    std::mt19937_64                        rng(42);
    std::uniform_real_distribution<double> dist(-1000.0, 1000.0);

    for (auto& val : data)
    {
        val = dist(rng);
    }

    for (auto _ : state)
    {
        double result = parallel_reduce(
            0,
            work_size,
            grain_size,
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

        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * work_size);
}

// ============================================================================
// Overhead and Scaling Benchmarks
// ============================================================================

/**
 * @brief Measure parallelization overhead with minimal work
 */
static void BM_Overhead_MinimalWork(benchmark::State& state)
{
    const int64_t work_size   = state.range(0);
    const int     num_threads = state.range(1);

    set_num_threads(num_threads);

    std::atomic<int64_t> counter{0};

    for (auto _ : state)
    {
        counter.store(0, std::memory_order_relaxed);

        parallel_for(
            0,
            work_size,
            1,
            [&counter](int64_t begin, int64_t end)
            {
                // Minimal work - just increment counter
                counter.fetch_add(end - begin, std::memory_order_relaxed);
            });

        benchmark::DoNotOptimize(counter.load(std::memory_order_acquire));
    }

    state.SetLabel("Overhead_Threads=" + std::to_string(num_threads));
}

/**
 * @brief Thread scaling efficiency
 * Measures how efficiently the backend scales with increasing thread count
 */
static void BM_ThreadScaling_Efficiency(benchmark::State& state)
{
    const int64_t work_size   = 100000;
    const int     num_threads = state.range(0);

    set_num_threads(num_threads);

    for (auto _ : state)
    {
        parallel_for(
            0,
            work_size,
            work_size / (num_threads * 4),
            [](int64_t begin, int64_t end)
            {
                volatile double sum = 0.0;
                for (int64_t i = begin; i < end; ++i)
                {
                    sum += std::sin(static_cast<double>(i) * 0.001) *
                           std::cos(static_cast<double>(i) * 0.001);
                }
                benchmark::DoNotOptimize(sum);
            });
    }

    state.SetItemsProcessed(state.iterations() * work_size);
}

// ============================================================================
// Backend-Specific API Benchmarks
// ============================================================================

/**
 * @brief Benchmark smp_tools API (backend-agnostic interface)
 */
static void BM_SMPTools_ParallelFor(benchmark::State& state)
{
    const int64_t work_size   = state.range(0);
    const int     num_threads = state.range(1);

    smp_tools::initialize(num_threads);

    std::atomic<double> result{0.0};

    for (auto _ : state)
    {
        result.store(0.0, std::memory_order_relaxed);

        smp_tools::For(
            0,
            work_size,
            [&result](int64_t begin, int64_t end)
            {
                double local_sum =
                    benchmark_detail::compute_heavy_numerical_integration(begin, end);
                double current = result.load(std::memory_order_relaxed);
                while (!result.compare_exchange_weak(
                    current,
                    current + local_sum,
                    std::memory_order_release,
                    std::memory_order_relaxed))
                {
                }
            });

        benchmark::DoNotOptimize(result.load(std::memory_order_acquire));
    }

    state.SetItemsProcessed(state.iterations() * work_size);
}

// ============================================================================
// Benchmark Registration with Various Configurations
// ============================================================================

// Small workload: Test overhead and fine-grained parallelism
BENCHMARK(BM_ParallelFor_Integration)
    ->Args({1000, 100, 1})  // Single-threaded baseline
    ->Args({1000, 100, 2})  // 2 threads
    ->Args({1000, 100, 4})  // 4 threads
    ->Args({1000, 100, 8})  // 8 threads
    ->MinTime(0.5);

// Medium workload: Balanced workload testing
BENCHMARK(BM_ParallelFor_Integration)
    ->Args({50000, 1000, 1})
    ->Args({50000, 1000, 2})
    ->Args({50000, 1000, 4})
    ->Args({50000, 1000, 8})
    ->Args({50000, 1000, 16})
    ->MinTime(1.0);

// Large workload: Scalability testing
BENCHMARK(BM_ParallelFor_Integration)
    ->Args({500000, 10000, 1})
    ->Args({500000, 10000, 2})
    ->Args({500000, 10000, 4})
    ->Args({500000, 10000, 8})
    ->Args({500000, 10000, 16})
    ->MinTime(2.0);

// Matrix multiplication benchmarks
BENCHMARK(BM_ParallelFor_MatrixMult)
    ->Args({10000, 500, 1})
    ->Args({10000, 500, 2})
    ->Args({10000, 500, 4})
    ->Args({10000, 500, 8})
    ->MinTime(1.0);

// Prime checking (irregular workload)
BENCHMARK(BM_ParallelFor_PrimeCheck)
    ->Args({10000, 500, 1})
    ->Args({10000, 500, 2})
    ->Args({10000, 500, 4})
    ->Args({10000, 500, 8})
    ->MinTime(1.0);

// Parallel reduce benchmarks
BENCHMARK(BM_ParallelReduce_Sum)
    ->Args({1000000, 10000, 1})
    ->Args({1000000, 10000, 2})
    ->Args({1000000, 10000, 4})
    ->Args({1000000, 10000, 8})
    ->MinTime(1.0);

BENCHMARK(BM_ParallelReduce_Max)
    ->Args({1000000, 10000, 1})
    ->Args({1000000, 10000, 2})
    ->Args({1000000, 10000, 4})
    ->Args({1000000, 10000, 8})
    ->MinTime(1.0);

// Overhead benchmarks
BENCHMARK(BM_Overhead_MinimalWork)
    ->Args({10000, 1})
    ->Args({10000, 2})
    ->Args({10000, 4})
    ->Args({10000, 8})
    ->MinTime(0.5);

// Thread scaling
BENCHMARK(BM_ThreadScaling_Efficiency)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16)->MinTime(1.0);

// SMP Tools API
BENCHMARK(BM_SMPTools_ParallelFor)
    ->Args({50000, 1})
    ->Args({50000, 2})
    ->Args({50000, 4})
    ->Args({50000, 8})
    ->MinTime(1.0);

}  // namespace xsigma

BENCHMARK_MAIN();
