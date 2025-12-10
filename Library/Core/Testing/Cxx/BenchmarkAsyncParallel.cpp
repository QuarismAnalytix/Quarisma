/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Benchmarks for async_parallel_for and async_parallel_reduce
 * Measures overhead, latency, throughput, and compares with synchronous versions
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

#include "parallel/parallel.h"

namespace xsigma
{

// ============================================================================
// Benchmark Group 1: Async Launch Overhead
// ============================================================================

// Benchmark: Overhead of launching async_parallel_for
static void BM_AsyncParallelFor_LaunchOverhead(benchmark::State& state)
{
    const int64_t    size = state.range(0);
    std::vector<int> data(size, 0);

    for (auto _ : state)
    {
        auto handle = async_parallel_for(
            0,
            size,
            size / 10,
            [&data](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    data[i] = static_cast<int>(i);
                }
            });

        // Don't wait - measure launch overhead only
        benchmark::DoNotOptimize(handle);
        handle.wait();  // Clean up for next iteration
    }

    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_AsyncParallelFor_LaunchOverhead)->Range(1000, 1000000);

// Benchmark: Overhead of launching async_parallel_reduce
static void BM_AsyncParallelReduce_LaunchOverhead(benchmark::State& state)
{
    const int64_t    size = state.range(0);
    std::vector<int> data(size, 1);

    for (auto _ : state)
    {
        auto handle = async_parallel_reduce(
            0,
            size,
            size / 10,
            0,
            [&data](int64_t begin, int64_t end, int identity)
            {
                int sum = identity;
                for (int64_t i = begin; i < end; ++i)
                {
                    sum += data[i];
                }
                return sum;
            },
            [](int a, int b) { return a + b; });

        benchmark::DoNotOptimize(handle);
        handle.wait();  // Clean up
    }

    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_AsyncParallelReduce_LaunchOverhead)->Range(1000, 1000000);

// ============================================================================
// Benchmark Group 2: Sync vs Async Comparison - parallel_for
// ============================================================================

// Benchmark: Synchronous parallel_for
static void BM_ParallelFor_Sync(benchmark::State& state)
{
    const int64_t    size       = state.range(0);
    const int64_t    grain_size = state.range(1);
    std::vector<int> data(size, 0);

    for (auto _ : state)
    {
        parallel_for(
            0,
            size,
            grain_size,
            [&data](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    // Simulate work
                    data[i] = static_cast<int>(std::sqrt(static_cast<double>(i)));
                }
            });
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_ParallelFor_Sync)->Args({10000, 1000})->Args({100000, 10000})->Args({1000000, 100000});

// Benchmark: Asynchronous parallel_for
static void BM_ParallelFor_Async(benchmark::State& state)
{
    const int64_t    size       = state.range(0);
    const int64_t    grain_size = state.range(1);
    std::vector<int> data(size, 0);

    for (auto _ : state)
    {
        auto handle = async_parallel_for(
            0,
            size,
            grain_size,
            [&data](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    // Simulate work
                    data[i] = static_cast<int>(std::sqrt(static_cast<double>(i)));
                }
            });
        handle.wait();
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_ParallelFor_Async)
    ->Args({10000, 1000})
    ->Args({100000, 10000})
    ->Args({1000000, 100000});

// ============================================================================
// Benchmark Group 3: Sync vs Async Comparison - parallel_reduce
// ============================================================================

// Benchmark: Synchronous parallel_reduce
static void BM_ParallelReduce_Sync(benchmark::State& state)
{
    const int64_t       size       = state.range(0);
    const int64_t       grain_size = state.range(1);
    std::vector<double> data(size);
    for (int64_t i = 0; i < size; ++i)
    {
        data[i] = static_cast<double>(i);
    }

    for (auto _ : state)
    {
        double result = parallel_reduce(
            0,
            size,
            grain_size,
            0.0,
            [&data](int64_t begin, int64_t end, double identity)
            {
                double sum = identity;
                for (int64_t i = begin; i < end; ++i)
                {
                    sum += std::sqrt(data[i]);
                }
                return sum;
            },
            [](double a, double b) { return a + b; });
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_ParallelReduce_Sync)
    ->Args({10000, 1000})
    ->Args({100000, 10000})
    ->Args({1000000, 100000});

// Benchmark: Asynchronous parallel_reduce
static void BM_ParallelReduce_Async(benchmark::State& state)
{
    const int64_t       size       = state.range(0);
    const int64_t       grain_size = state.range(1);
    std::vector<double> data(size);
    for (int64_t i = 0; i < size; ++i)
    {
        data[i] = static_cast<double>(i);
    }

    for (auto _ : state)
    {
        auto handle = async_parallel_reduce(
            0,
            size,
            grain_size,
            0.0,
            [&data](int64_t begin, int64_t end, double identity)
            {
                double sum = identity;
                for (int64_t i = begin; i < end; ++i)
                {
                    sum += std::sqrt(data[i]);
                }
                return sum;
            },
            [](double a, double b) { return a + b; });
        double result = handle.get();
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_ParallelReduce_Async)
    ->Args({10000, 1000})
    ->Args({100000, 10000})
    ->Args({1000000, 100000});

// ============================================================================
// Benchmark Group 4: Concurrent Async Operations
// ============================================================================

// Benchmark: Multiple concurrent async_parallel_for operations
static void BM_ConcurrentAsyncFor(benchmark::State& state)
{
    const int64_t num_ops     = state.range(0);
    const int64_t size_per_op = state.range(1);

    std::vector<std::vector<int>> data(num_ops);
    for (auto& vec : data)
    {
        vec.resize(size_per_op, 0);
    }

    for (auto _ : state)
    {
        std::vector<async_handle<void>> handles;
        handles.reserve(num_ops);

        // Launch all operations
        for (int64_t op = 0; op < num_ops; ++op)
        {
            handles.emplace_back(async_parallel_for(
                0,
                size_per_op,
                size_per_op / 10,
                [&data, op](int64_t begin, int64_t end)
                {
                    for (int64_t i = begin; i < end; ++i)
                    {
                        data[op][i] = static_cast<int>(i * op);
                    }
                }));
        }

        // Wait for all
        for (auto& handle : handles)
        {
            handle.wait();
        }

        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * num_ops * size_per_op);
}
BENCHMARK(BM_ConcurrentAsyncFor)->Args({4, 10000})->Args({8, 10000})->Args({16, 10000});

// Benchmark: Multiple concurrent async_parallel_reduce operations
static void BM_ConcurrentAsyncReduce(benchmark::State& state)
{
    const int64_t num_ops     = state.range(0);
    const int64_t size_per_op = state.range(1);

    std::vector<std::vector<int>> data(num_ops);
    for (auto& vec : data)
    {
        vec.resize(size_per_op, 1);
    }

    for (auto _ : state)
    {
        std::vector<async_handle<int64_t>> handles;
        handles.reserve(num_ops);

        // Launch all operations
        for (int64_t op = 0; op < num_ops; ++op)
        {
            handles.emplace_back(async_parallel_reduce(
                0,
                size_per_op,
                size_per_op / 10,
                0LL,
                [&data, op](int64_t begin, int64_t end, int64_t identity)
                {
                    int64_t sum = identity;
                    for (int64_t i = begin; i < end; ++i)
                    {
                        sum += data[op][i];
                    }
                    return sum;
                },
                [](int64_t a, int64_t b) { return a + b; }));
        }

        // Wait for all and collect results
        int64_t total = 0;
        for (auto& handle : handles)
        {
            total += handle.get();
        }

        benchmark::DoNotOptimize(total);
    }

    state.SetItemsProcessed(state.iterations() * num_ops * size_per_op);
}
BENCHMARK(BM_ConcurrentAsyncReduce)->Args({4, 10000})->Args({8, 10000})->Args({16, 10000});

// ============================================================================
// Benchmark Group 5: Realistic Workloads
// ============================================================================

// Benchmark: Matrix-vector multiplication (sync)
static void BM_MatrixVectorMult_Sync(benchmark::State& state)
{
    const int64_t rows = state.range(0);
    const int64_t cols = 1000;

    std::vector<std::vector<double>> matrix(rows, std::vector<double>(cols, 1.0));
    std::vector<double>              vector(cols, 1.0);
    std::vector<double>              result(rows, 0.0);

    for (auto _ : state)
    {
        parallel_for(
            0,
            rows,
            rows / 10,
            [&](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    double sum = 0.0;
                    for (int64_t j = 0; j < cols; ++j)
                    {
                        sum += matrix[i][j] * vector[j];
                    }
                    result[i] = sum;
                }
            });
        benchmark::DoNotOptimize(result.data());
    }

    state.SetItemsProcessed(state.iterations() * rows * cols);
}
BENCHMARK(BM_MatrixVectorMult_Sync)->Arg(1000)->Arg(10000)->Arg(100000);

// Benchmark: Matrix-vector multiplication (async)
static void BM_MatrixVectorMult_Async(benchmark::State& state)
{
    const int64_t rows = state.range(0);
    const int64_t cols = 1000;

    std::vector<std::vector<double>> matrix(rows, std::vector<double>(cols, 1.0));
    std::vector<double>              vector(cols, 1.0);
    std::vector<double>              result(rows, 0.0);

    for (auto _ : state)
    {
        auto handle = async_parallel_for(
            0,
            rows,
            rows / 10,
            [&](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    double sum = 0.0;
                    for (int64_t j = 0; j < cols; ++j)
                    {
                        sum += matrix[i][j] * vector[j];
                    }
                    result[i] = sum;
                }
            });
        handle.wait();
        benchmark::DoNotOptimize(result.data());
    }

    state.SetItemsProcessed(state.iterations() * rows * cols);
}
BENCHMARK(BM_MatrixVectorMult_Async)->Arg(1000)->Arg(10000)->Arg(100000);

// Benchmark: Data transformation pipeline (async)
static void BM_DataPipeline_Async(benchmark::State& state)
{
    const int64_t size = state.range(0);

    std::vector<double> input(size);
    std::vector<double> stage1(size);
    std::vector<double> stage2(size);
    std::vector<double> output(size);

    // Initialize input
    for (int64_t i = 0; i < size; ++i)
    {
        input[i] = static_cast<double>(i);
    }

    for (auto _ : state)
    {
        // Stage 1: sqrt
        auto h1 = async_parallel_for(
            0,
            size,
            size / 10,
            [&](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    stage1[i] = std::sqrt(input[i]);
                }
            });

        h1.wait();

        // Stage 2: square
        auto h2 = async_parallel_for(
            0,
            size,
            size / 10,
            [&](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    stage2[i] = stage1[i] * stage1[i];
                }
            });

        h2.wait();

        // Stage 3: log
        auto h3 = async_parallel_for(
            0,
            size,
            size / 10,
            [&](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    output[i] = std::log(stage2[i] + 1.0);
                }
            });

        h3.wait();
        benchmark::DoNotOptimize(output.data());
    }

    state.SetItemsProcessed(state.iterations() * size * 3);  // 3 stages
}
BENCHMARK(BM_DataPipeline_Async)->Arg(10000)->Arg(100000)->Arg(1000000);

// ============================================================================
// Benchmark Group 6: Grain Size Impact
// ============================================================================

// Benchmark: Impact of grain size on async_parallel_for
static void BM_GrainSize_AsyncFor(benchmark::State& state)
{
    const int64_t    size       = 100000;
    const int64_t    grain_size = state.range(0);
    std::vector<int> data(size, 0);

    for (auto _ : state)
    {
        auto handle = async_parallel_for(
            0,
            size,
            grain_size,
            [&data](int64_t begin, int64_t end)
            {
                for (int64_t i = begin; i < end; ++i)
                {
                    data[i] = static_cast<int>(i);
                }
            });
        handle.wait();
        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_GrainSize_AsyncFor)->Arg(100)->Arg(1000)->Arg(10000)->Arg(50000);

// Benchmark: Impact of grain size on async_parallel_reduce
static void BM_GrainSize_AsyncReduce(benchmark::State& state)
{
    const int64_t    size       = 100000;
    const int64_t    grain_size = state.range(0);
    std::vector<int> data(size, 1);

    for (auto _ : state)
    {
        auto handle = async_parallel_reduce(
            0,
            size,
            grain_size,
            0,
            [&data](int64_t begin, int64_t end, int identity)
            {
                int sum = identity;
                for (int64_t i = begin; i < end; ++i)
                {
                    sum += data[i];
                }
                return sum;
            },
            [](int a, int b) { return a + b; });
        int result = handle.get();
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * size);
}
BENCHMARK(BM_GrainSize_AsyncReduce)->Arg(100)->Arg(1000)->Arg(10000)->Arg(50000);

// ============================================================================
// Benchmark Group 7: Throughput Tests
// ============================================================================

// Benchmark: Throughput of many small async operations
static void BM_Throughput_SmallTasks(benchmark::State& state)
{
    const int64_t num_tasks = state.range(0);
    const int64_t task_size = 100;

    std::vector<std::vector<int>> data(num_tasks, std::vector<int>(task_size, 0));

    for (auto _ : state)
    {
        std::vector<async_handle<void>> handles;
        handles.reserve(num_tasks);

        for (int64_t t = 0; t < num_tasks; ++t)
        {
            handles.emplace_back(async_parallel_for(
                0,
                task_size,
                task_size,
                [&data, t](int64_t begin, int64_t end)
                {
                    for (int64_t i = begin; i < end; ++i)
                    {
                        data[t][i] = static_cast<int>(i);
                    }
                }));
        }

        for (auto& handle : handles)
        {
            handle.wait();
        }

        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * num_tasks * task_size);
}
BENCHMARK(BM_Throughput_SmallTasks)->Arg(10)->Arg(50)->Arg(100)->Arg(200);

// Benchmark: Throughput of few large async operations
static void BM_Throughput_LargeTasks(benchmark::State& state)
{
    const int64_t num_tasks = state.range(0);
    const int64_t task_size = 100000;

    std::vector<std::vector<int>> data(num_tasks, std::vector<int>(task_size, 0));

    for (auto _ : state)
    {
        std::vector<async_handle<void>> handles;
        handles.reserve(num_tasks);

        for (int64_t t = 0; t < num_tasks; ++t)
        {
            handles.emplace_back(async_parallel_for(
                0,
                task_size,
                task_size / 10,
                [&data, t](int64_t begin, int64_t end)
                {
                    for (int64_t i = begin; i < end; ++i)
                    {
                        data[t][i] = static_cast<int>(i);
                    }
                }));
        }

        for (auto& handle : handles)
        {
            handle.wait();
        }

        benchmark::DoNotOptimize(data.data());
    }

    state.SetItemsProcessed(state.iterations() * num_tasks * task_size);
}
BENCHMARK(BM_Throughput_LargeTasks)->Arg(2)->Arg(4)->Arg(8)->Arg(16);

}  // namespace xsigma

// Run benchmarks
BENCHMARK_MAIN();
