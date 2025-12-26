/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Comprehensive benchmark comparing smp vs smp_new performance
 * Tests across different workload types, data sizes, and grain sizes
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

#include "smp/tools.h"
#include "smp_new/parallel/parallel_api.h"

namespace xsigma
{

// ============================================================================
// Benchmark Group 1: Memory-Bound Operations (Simple Array Writes)
// ============================================================================

// Small dataset (100 items)
static void BM_SMP_MemoryBound_Small(benchmark::State& state)
{
    const int        size = 100;
    std::vector<int> data(size, 0);

    for (auto _ : state)
    {
        tools::For(
            0,
            size,
            10,
            [&data](int begin, int end)
            {
                for (int i = begin; i < end; ++i)
                {
                    data[i] = i;
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * size);
    state.SetBytesProcessed(state.iterations() * size * sizeof(int));
}
BENCHMARK(BM_SMP_MemoryBound_Small);

static void BM_SMPNew_MemoryBound_Small(benchmark::State& state)
{
    const int        size = 100;
    std::vector<int> data(size, 0);

    for (auto _ : state)
    {
        smp_new::parallel::parallel_for(
            0,
            size,
            10,
            [&data](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    data[i] = static_cast<int>(i);
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * size);
    state.SetBytesProcessed(state.iterations() * size * sizeof(int));
}
BENCHMARK(BM_SMPNew_MemoryBound_Small);

// Medium dataset (10,000 items)
static void BM_SMP_MemoryBound_Medium(benchmark::State& state)
{
    const int        size = 10000;
    std::vector<int> data(size, 0);

    for (auto _ : state)
    {
        tools::For(
            0,
            size,
            100,
            [&data](int begin, int end)
            {
                for (int i = begin; i < end; ++i)
                {
                    data[i] = i;
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * size);
    state.SetBytesProcessed(state.iterations() * size * sizeof(int));
}
BENCHMARK(BM_SMP_MemoryBound_Medium);

static void BM_SMPNew_MemoryBound_Medium(benchmark::State& state)
{
    const int        size = 10000;
    std::vector<int> data(size, 0);

    for (auto _ : state)
    {
        smp_new::parallel::parallel_for(
            0,
            size,
            100,
            [&data](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    data[i] = static_cast<int>(i);
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * size);
    state.SetBytesProcessed(state.iterations() * size * sizeof(int));
}
BENCHMARK(BM_SMPNew_MemoryBound_Medium);

// Large dataset (1,000,000 items)
static void BM_SMP_MemoryBound_Large(benchmark::State& state)
{
    const int        size = 1000000;
    std::vector<int> data(size, 0);

    for (auto _ : state)
    {
        tools::For(
            0,
            size,
            10000,
            [&data](int begin, int end)
            {
                for (int i = begin; i < end; ++i)
                {
                    data[i] = i;
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * size);
    state.SetBytesProcessed(state.iterations() * size * sizeof(int));
}
BENCHMARK(BM_SMP_MemoryBound_Large);

static void BM_SMPNew_MemoryBound_Large(benchmark::State& state)
{
    const int        size = 1000000;
    std::vector<int> data(size, 0);

    for (auto _ : state)
    {
        smp_new::parallel::parallel_for(
            0,
            size,
            10000,
            [&data](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    data[i] = static_cast<int>(i);
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * size);
    state.SetBytesProcessed(state.iterations() * size * sizeof(int));
}
BENCHMARK(BM_SMPNew_MemoryBound_Large);

// ============================================================================
// Benchmark Group 2: Compute-Bound Operations (Mathematical Calculations)
// ============================================================================

// Small dataset with heavy computation
static void BM_SMP_ComputeBound_Small(benchmark::State& state)
{
    const int           size = 1000;
    std::vector<double> data(size, 0.0);

    for (auto _ : state)
    {
        tools::For(
            0,
            size,
            50,
            [&data](int begin, int end)
            {
                for (int i = begin; i < end; ++i)
                {
                    double x = static_cast<double>(i) * 0.01;
                    data[i]  = std::sin(x) * std::cos(x) + std::sqrt(x + 1.0);
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_SMP_ComputeBound_Small);

static void BM_SMPNew_ComputeBound_Small(benchmark::State& state)
{
    const int           size = 1000;
    std::vector<double> data(size, 0.0);

    for (auto _ : state)
    {
        smp_new::parallel::parallel_for(
            0,
            size,
            50,
            [&data](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    double x = static_cast<double>(i) * 0.01;
                    data[i]  = std::sin(x) * std::cos(x) + std::sqrt(x + 1.0);
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_SMPNew_ComputeBound_Small);

// Medium dataset with heavy computation
static void BM_SMP_ComputeBound_Medium(benchmark::State& state)
{
    const int           size = 50000;
    std::vector<double> data(size, 0.0);

    for (auto _ : state)
    {
        tools::For(
            0,
            size,
            500,
            [&data](int begin, int end)
            {
                for (int i = begin; i < end; ++i)
                {
                    double x = static_cast<double>(i) * 0.01;
                    data[i]  = std::sin(x) * std::cos(x) + std::sqrt(x + 1.0);
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_SMP_ComputeBound_Medium);

static void BM_SMPNew_ComputeBound_Medium(benchmark::State& state)
{
    const int           size = 50000;
    std::vector<double> data(size, 0.0);

    for (auto _ : state)
    {
        smp_new::parallel::parallel_for(
            0,
            size,
            500,
            [&data](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    double x = static_cast<double>(i) * 0.01;
                    data[i]  = std::sin(x) * std::cos(x) + std::sqrt(x + 1.0);
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_SMPNew_ComputeBound_Medium);

// ============================================================================
// Benchmark Group 3: Reduction Operations
// ============================================================================

// Sum reduction - small
static void BM_SMP_Reduce_Sum_Small(benchmark::State& state)
{
    const int        size = 1000;
    std::vector<int> data(size);
    std::iota(data.begin(), data.end(), 0);

    for (auto _ : state)
    {
        int64_t sum = 0;
        tools::For(
            0,
            size,
            100,
            [&data, &sum](int begin, int end)
            {
                int64_t local_sum = 0;
                for (int i = begin; i < end; ++i)
                {
                    local_sum += data[i];
                }
                // Note: This has a race condition, but for benchmark purposes
                sum += local_sum;
            });
        benchmark::DoNotOptimize(sum);
    }

    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_SMP_Reduce_Sum_Small);

static void BM_SMPNew_Reduce_Sum_Small(benchmark::State& state)
{
    const int        size = 1000;
    std::vector<int> data(size);
    std::iota(data.begin(), data.end(), 0);

    for (auto _ : state)
    {
        int64_t sum = smp_new::parallel::parallel_reduce(
            0,
            size,
            100,
            static_cast<int64_t>(0),
            [&data](int64_t begin, int64_t end, int64_t init) -> int64_t
            {
                int64_t local_sum = init;
                for (int64_t i = begin; i < end; ++i)
                {
                    local_sum += data[i];
                }
                return local_sum;
            },
            [](int64_t a, int64_t b) -> int64_t { return a + b; });
        benchmark::DoNotOptimize(sum);
    }

    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_SMPNew_Reduce_Sum_Small);

// Sum reduction - large
static void BM_SMP_Reduce_Sum_Large(benchmark::State& state)
{
    const int        size = 1000000;
    std::vector<int> data(size);
    std::iota(data.begin(), data.end(), 0);

    for (auto _ : state)
    {
        int64_t sum = 0;
        tools::For(
            0,
            size,
            10000,
            [&data, &sum](int begin, int end)
            {
                int64_t local_sum = 0;
                for (int i = begin; i < end; ++i)
                {
                    local_sum += data[i];
                }
                sum += local_sum;
            });
        benchmark::DoNotOptimize(sum);
    }

    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_SMP_Reduce_Sum_Large);

static void BM_SMPNew_Reduce_Sum_Large(benchmark::State& state)
{
    const int        size = 1000000;
    std::vector<int> data(size);
    std::iota(data.begin(), data.end(), 0);

    for (auto _ : state)
    {
        int64_t sum = smp_new::parallel::parallel_reduce(
            0,
            size,
            10000,
            static_cast<int64_t>(0),
            [&data](int64_t begin, int64_t end, int64_t init) -> int64_t
            {
                int64_t local_sum = init;
                for (int64_t i = begin; i < end; ++i)
                {
                    local_sum += data[i];
                }
                return local_sum;
            },
            [](int64_t a, int64_t b) -> int64_t { return a + b; });
        benchmark::DoNotOptimize(sum);
    }

    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_SMPNew_Reduce_Sum_Large);

// ============================================================================
// Benchmark Group 4: Transform Operations
// ============================================================================

// Transform - small
static void BM_SMP_Transform_Small(benchmark::State& state)
{
    const int        size = 1000;
    std::vector<int> input(size);
    std::iota(input.begin(), input.end(), 0);
    std::vector<int> output(size, 0);

    for (auto _ : state)
    {
        tools::Transform(
            input.begin(), input.end(), output.begin(), [](int x) { return x * 2 + 1; });
        benchmark::DoNotOptimize(output.data());
    }

    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_SMP_Transform_Small);

static void BM_SMPNew_Transform_Small(benchmark::State& state)
{
    const int        size = 1000;
    std::vector<int> input(size);
    std::iota(input.begin(), input.end(), 0);
    std::vector<int> output(size, 0);

    for (auto _ : state)
    {
        smp_new::parallel::parallel_for(
            0,
            size,
            100,
            [&input, &output](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    output[i] = input[i] * 2 + 1;
                }
            });
        benchmark::DoNotOptimize(output.data());
    }

    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_SMPNew_Transform_Small);

}  // namespace xsigma

BENCHMARK_MAIN();
