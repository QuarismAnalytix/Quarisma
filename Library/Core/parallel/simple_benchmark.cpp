/**
 * @file simple_benchmark.cpp
 * @brief Simple side-by-side benchmark: sequential vs XSigma vs OpenMP vs TBB
 *
 * COMPILATION INSTRUCTIONS:
 * =========================
 *
 * 1. sequential + XSigma Native Backend (no dependencies):
 *    g++ -std=c++17 -O3 -I../.. simple_benchmark.cpp \
 *        ../../../build/lib/libxsigma_core.a -lpthread -o benchmark
 *
 * 2. With OpenMP:
 *    g++ -std=c++17 -O3 -fopenmp -I../.. simple_benchmark.cpp \
 *        ../../../build/lib/libxsigma_core.a -lpthread -o benchmark
 *
 * 3. With TBB:
 *    g++ -std=c++17 -O3 -I../.. simple_benchmark.cpp \
 *        ../../../build/lib/libxsigma_core.a -ltbb -lpthread -o benchmark
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

// Include XSigma parallel
#include "parallel/parallel.h"

// OpenMP (if available)
#ifdef _OPENMP
#include <omp.h>
#endif

// TBB (if available)
#if XSIGMA_HAS_TBB
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#endif

// ============================================================================
// Timer Utility
// ============================================================================

class Timer
{
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}

    double elapsed_ms() const
    {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }

    void reset() { start_ = std::chrono::high_resolution_clock::now(); }

private:
    std::chrono::high_resolution_clock::time_point start_;
};

// ============================================================================
// Benchmark Kernel: Expensive computation per element
// ============================================================================

inline double compute_expensive(int64_t i)
{
    // Simulates expensive computation: ~50-100 CPU cycles
    double x = i * 0.001;
    return std::sin(x) + std::cos(x) + std::sqrt(std::abs(x));
}

// ============================================================================
// APPROACH 1: sequential
// ============================================================================

double benchmark_sequential(int64_t N)
{
    std::vector<double> data(N);

    Timer timer;

    // Simple sequential for loop
    for (int64_t i = 0; i < N; ++i)
    {
        data[i] = compute_expensive(i);
    }

    double elapsed = timer.elapsed_ms();

    // Compute checksum for verification
    double sum = std::accumulate(data.begin(), data.end(), 0.0);

    std::cout << "  sequential:       " << std::setw(10) << std::fixed << std::setprecision(2)
              << elapsed << " ms  (checksum: " << std::setprecision(6) << sum << ")\n";

    return elapsed;
}

// ============================================================================
// APPROACH 2: XSigma parallel_for
// ============================================================================

double benchmark_xsigma(int64_t N, int64_t grain_size)
{
    std::vector<double> data(N);

    Timer timer;

    // XSigma parallel_for
    xsigma::parallel_for(0, N, grain_size, [&data](int64_t begin, int64_t end) {
        for (int64_t i = begin; i < end; ++i)
        {
            data[i] = compute_expensive(i);
        }
    });

    double elapsed = timer.elapsed_ms();

    // Compute checksum for verification
    double sum = std::accumulate(data.begin(), data.end(), 0.0);

    std::cout << "  XSigma:           " << std::setw(10) << std::fixed << std::setprecision(2)
              << elapsed << " ms  (checksum: " << std::setprecision(6) << sum << ")\n";

    return elapsed;
}

// ============================================================================
// APPROACH 3: Native OpenMP parallel for
// ============================================================================

#ifdef _OPENMP
double benchmark_openmp(int64_t N)
{
    std::vector<double> data(N);

    Timer timer;

    // Native OpenMP parallel for with static scheduling
#pragma omp parallel for schedule(static)
    for (int64_t i = 0; i < N; ++i)
    {
        data[i] = compute_expensive(i);
    }

    double elapsed = timer.elapsed_ms();

    // Compute checksum for verification
    double sum = std::accumulate(data.begin(), data.end(), 0.0);

    std::cout << "  OpenMP (static):  " << std::setw(10) << std::fixed << std::setprecision(2)
              << elapsed << " ms  (checksum: " << std::setprecision(6) << sum << ")\n";

    return elapsed;
}

double benchmark_openmp_dynamic(int64_t N, int64_t chunk_size)
{
    std::vector<double> data(N);

    Timer timer;

    // Native OpenMP parallel for with dynamic scheduling
#pragma omp parallel for schedule(dynamic, chunk_size)
    for (int64_t i = 0; i < N; ++i)
    {
        data[i] = compute_expensive(i);
    }

    double elapsed = timer.elapsed_ms();

    // Compute checksum for verification
    double sum = std::accumulate(data.begin(), data.end(), 0.0);

    std::cout << "  OpenMP (dynamic): " << std::setw(10) << std::fixed << std::setprecision(2)
              << elapsed << " ms  (checksum: " << std::setprecision(6) << sum << ")\n";

    return elapsed;
}
#endif

// ============================================================================
// APPROACH 4: Native Intel TBB parallel_for
// ============================================================================

#if XSIGMA_HAS_TBB
double benchmark_tbb(int64_t N, int64_t grain_size)
{
    std::vector<double> data(N);

    Timer timer;

    // Native TBB parallel_for
    tbb::parallel_for(
        tbb::blocked_range<int64_t>(0, N, grain_size),
        [&data](const tbb::blocked_range<int64_t>& range) {
            for (int64_t i = range.begin(); i < range.end(); ++i)
            {
                data[i] = compute_expensive(i);
            }
        });

    double elapsed = timer.elapsed_ms();

    // Compute checksum for verification
    double sum = std::accumulate(data.begin(), data.end(), 0.0);

    std::cout << "  TBB:              " << std::setw(10) << std::fixed << std::setprecision(2)
              << elapsed << " ms  (checksum: " << std::setprecision(6) << sum << ")\n";

    return elapsed;
}
#endif

// ============================================================================
// Main Benchmark
// ============================================================================

void run_benchmark(int64_t N)
{
    // Compute optimal grain_size
    int num_threads = xsigma::get_num_threads();
    int64_t grain_size = std::max<int64_t>(1000, N / (num_threads * 100));

    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Benchmark: N = " << N << " elements\n";
    std::cout << "Grain size: " << grain_size << "\n";
    std::cout << "Hardware threads: " << std::thread::hardware_concurrency() << "\n";
    std::cout << "Active threads: " << num_threads << "\n";
    std::cout << std::string(70, '=') << "\n";

    // Run benchmarks
    double seq_time = benchmark_sequential(N);
    double xsigma_time = benchmark_xsigma(N, grain_size);

#ifdef _OPENMP
    double omp_time = benchmark_openmp(N);
    double omp_dyn_time = benchmark_openmp_dynamic(N, grain_size);
#endif

#if XSIGMA_HAS_TBB
    double tbb_time = benchmark_tbb(N, grain_size);
#endif

    // Print speedups
    std::cout << "\nSpeedups (vs sequential):\n";
    std::cout << "  XSigma:           " << std::fixed << std::setprecision(2)
              << (seq_time / xsigma_time) << "x\n";

#ifdef _OPENMP
    std::cout << "  OpenMP (static):  " << (seq_time / omp_time) << "x\n";
    std::cout << "  OpenMP (dynamic): " << (seq_time / omp_dyn_time) << "x\n";
#endif

#if XSIGMA_HAS_TBB
    std::cout << "  TBB:              " << (seq_time / tbb_time) << "x\n";
#endif

    // Print efficiency (speedup / num_threads)
    std::cout << "\nParallel Efficiency (speedup / threads):\n";
    std::cout << "  XSigma:           " << std::fixed << std::setprecision(1)
              << ((seq_time / xsigma_time) / num_threads * 100.0) << "%\n";

#ifdef _OPENMP
    std::cout << "  OpenMP (static):  " << ((seq_time / omp_time) / num_threads * 100.0) << "%\n";
    std::cout << "  OpenMP (dynamic): " << ((seq_time / omp_dyn_time) / num_threads * 100.0)
              << "%\n";
#endif

#if XSIGMA_HAS_TBB
    std::cout << "  TBB:              " << ((seq_time / tbb_time) / num_threads * 100.0) << "%\n";
#endif
}

int main()
{
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  XSigma Parallel Execution Framework - Benchmark              ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";

    // Print backend information
    std::cout << "\nBackend Information:\n";
    std::cout << xsigma::get_parallel_info() << "\n";

    // Run benchmarks with different sizes
    std::vector<int64_t> sizes = {
        10'000,      // 10K - small (parallelism may not help)
        100'000,     // 100K - medium (parallelism starts to help)
        1'000'000,   // 1M - large (parallelism definitely helps)
        10'000'000,  // 10M - very large (maximum benefit)
    };

    for (int64_t N : sizes)
    {
        run_benchmark(N);
    }

    // Grain size tuning experiment
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Grain Size Tuning (N = 1,000,000)\n";
    std::cout << std::string(70, '=') << "\n";

    const int64_t N = 1'000'000;
    std::vector<int64_t> grain_sizes = {100, 1'000, 10'000, 100'000};

    for (int64_t grain : grain_sizes)
    {
        std::vector<double> data(N);
        Timer timer;

        xsigma::parallel_for(0, N, grain, [&data](int64_t begin, int64_t end) {
            for (int64_t i = begin; i < end; ++i)
            {
                data[i] = compute_expensive(i);
            }
        });

        double elapsed = timer.elapsed_ms();
        int64_t num_chunks = (N + grain - 1) / grain;

        std::cout << "  grain_size = " << std::setw(8) << grain << "  =>  " << std::setw(8)
                  << num_chunks << " chunks  =>  " << std::setw(10) << std::fixed
                  << std::setprecision(2) << elapsed << " ms\n";
    }

    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Benchmark Complete                                            ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";

    return 0;
}
