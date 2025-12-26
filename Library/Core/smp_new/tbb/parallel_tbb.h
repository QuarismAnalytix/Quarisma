#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "common/macros.h"

#ifndef QUARISMA_HAS_TBB
#define QUARISMA_HAS_TBB 0
#endif

#if QUARISMA_HAS_TBB
#include <tbb/blocked_range.h>
#include <tbb/parallel_reduce.h>
#endif

#include <atomic>

namespace quarisma::smp_new::tbb
{

/**
 * @brief Intel Threading Building Blocks (TBB) parallel execution backend.
 *
 * This module provides TBB-based parallel execution for Quarisma's
 * parallel APIs, offering high-performance task-based parallelism.
 *
 * Features:
 * - Task-based parallelism with work-stealing scheduler
 * - Automatic load balancing
 * - Nested parallelism support
 * - Cache-aware scheduling
 * - Integration with TBB's parallel algorithms
 * - Scalable performance on multi-core systems
 * - Exception propagation
 *
 * Requirements:
 * - Intel TBB library at compile time
 * - CMake configuration with QUARISMA_HAS_TBB=1
 *
 * Usage:
 * @code
 * // Initialize TBB backend
 * quarisma::smp_new::parallel::set_backend(3);  // TBB
 *
 * // Use parallel_for with TBB
 * quarisma::smp_new::parallel::parallel_for(
 *     0, 1000, 100,
 *     [](int64_t begin, int64_t end) {
 *         // Parallel work
 *     }
 * );
 * @endcode
 */

/**
 * @brief Initialize the TBB parallel backend.
 *
 * This function initializes TBB's task scheduler and sets up
 * the thread pool for parallel execution.
 *
 * @note Thread-safe. Can be called multiple times (idempotent).
 * @note Requires TBB support at compile time (QUARISMA_HAS_TBB=1).
 */
QUARISMA_API void InitializeTBBBackend();

/**
 * @brief Shutdown the TBB parallel backend.
 *
 * This function shuts down TBB's task scheduler and releases resources.
 * After calling this function, the backend must be reinitialized before
 * using parallel APIs.
 *
 * @note Thread-safe. Can be called multiple times.
 */
QUARISMA_API void ShutdownTBBBackend();

/**
 * @brief Check if the TBB backend is initialized.
 *
 * @return true if the TBB backend is initialized, false otherwise.
 */
QUARISMA_API bool IsTBBBackendInitialized();

/**
 * @brief Check if TBB is available.
 *
 * @return true if TBB support is compiled in, false otherwise.
 */
QUARISMA_API bool IsTBBAvailable();

/**
 * @brief Set the number of TBB worker threads.
 *
 * This function configures the number of threads in TBB's task scheduler.
 *
 * @param nthreads Number of threads to use. Must be positive.
 *
 * @note Thread-safe.
 * @note Requires TBB support at compile time.
 */
QUARISMA_API void SetNumTBBThreads(int nthreads);

/**
 * @brief Get the number of TBB worker threads.
 *
 * @return The number of threads configured for TBB, or -1 if not initialized.
 */
QUARISMA_API int GetNumTBBThreads();

/**
 * @brief Get the current TBB thread ID.
 *
 * @return The thread ID within the TBB task arena, or 0 if not in a TBB task.
 */
QUARISMA_API int GetTBBThreadNum();

/**
 * @brief Check if currently executing within a TBB parallel region.
 *
 * @return true if inside a TBB parallel region, false otherwise.
 */
QUARISMA_API bool InTBBParallelRegion();

/**
 * @brief Get information about the TBB backend.
 *
 * @return A string containing TBB backend information (version, configuration, etc.)
 */
QUARISMA_API std::string GetTBBBackendInfo();

/**
 * @brief Get the thread-local parallel region flag for TBB.
 *
 * @return true if currently in a TBB parallel region, false otherwise.
 *
 * @note This function is always available and works on all platforms.
 *       When TBB is not available, it returns false.
 */
QUARISMA_API bool GetInParallelRegion();

/**
 * @brief Set the thread-local parallel region flag for TBB.
 *
 * @param in_region true if entering a parallel region, false if exiting.
 *
 * @note This function is always available and works on all platforms.
 *       When TBB is not available, it is a no-op.
 */
QUARISMA_API void SetInParallelRegion(bool in_region);
/**
 * @brief Execute a parallel for loop using TBB.
 *
 * This function divides the iteration space [begin, end) into chunks
 * and executes them in parallel using TBB's parallel_for algorithm.
 *
 * @param begin Start of iteration range (inclusive).
 * @param end End of iteration range (exclusive).
 * @param grain_size Minimum chunk size for parallel execution.
 *                   If grain_size <= 0, automatically calculates an appropriate grain size
 *                   based on the range size and number of threads (n / (num_threads * 4)).
 * @param func Function to execute for each chunk. Signature: void(int64_t begin, int64_t end)
 *
 * @note Thread-safe.
 * @note Requires TBB support at compile time.
 */
QUARISMA_API void ParallelForTBB(
    int64_t                                      begin,
    int64_t                                      end,
    int64_t                                      grain_size,
    const std::function<void(int64_t, int64_t)>& func);

/**
 * @brief Execute a parallel reduce operation using TBB.
 *
 * This function performs a parallel reduction over the range [begin, end)
 * using TBB's parallel_reduce algorithm.
 *
 * @tparam T The type of the reduction result.
 * @param begin Start of iteration range (inclusive).
 * @param end End of iteration range (exclusive).
 * @param grain_size Minimum chunk size for parallel execution.
 *                   If grain_size <= 0, automatically calculates an appropriate grain size
 *                   based on the range size and number of threads (n / (num_threads * 4)).
 * @param ident Identity value for the reduction operation.
 * @param func Reduction function. Signature: T(int64_t begin, int64_t end, T ident)
 * @param reduce Combine function. Signature: T(T a, T b)
 *
 * @return The result of the parallel reduction.
 *
 * @note Thread-safe.
 * @note Requires TBB support at compile time.
 */
#if QUARISMA_HAS_TBB
extern QUARISMA_API std::atomic<bool> g_tbb_initialized;
#endif

template <typename T>
T ParallelReduceTBB(
    int64_t                                      begin,
    int64_t                                      end,
    int64_t                                      grain_size,
    T                                            ident,
    const std::function<T(int64_t, int64_t, T)>& func,
    const std::function<T(T, T)>&                reduce)
{
#if QUARISMA_HAS_TBB
    // Initialize TBB backend if not already initialized
    if (!g_tbb_initialized.load())
    {
        InitializeTBBBackend();
    }

    int64_t n = end - begin;

    // Determine grain size if auto (grain_size <= 0)
    if (grain_size <= 0)
    {
        auto num_threads = static_cast<int64_t>(GetNumTBBThreads());
        if (num_threads <= 0)
        {
            num_threads = 1;
        }
        grain_size = std::max(static_cast<int64_t>(1), n / (num_threads * 4));
    }

    // Set parallel region flag
    bool was_in_parallel = GetInParallelRegion();
    SetInParallelRegion(true);

    // Execute parallel reduce
    // Note: TBB may throw exceptions internally, but we let them propagate
    // as this is a template function and TBB's exception handling is part
    // of its contract. The calling code should handle exceptions if needed.
    T result = ::tbb::parallel_reduce(
        ::tbb::blocked_range<int64_t>(begin, end, grain_size),
        ident,
        [&func](const ::tbb::blocked_range<int64_t>& range, T init) -> T
        { return func(range.begin(), range.end(), init); },
        [&reduce](T a, T b) -> T { return reduce(a, b); });

    // Restore parallel region flag
    SetInParallelRegion(was_in_parallel);
    return result;
#else
    // TBB is not available - fallback to serial execution
    (void)grain_size;  // Suppress unused parameter warning
    return func(begin, end, ident);
#endif
}
}  // namespace quarisma::smp_new::tbb