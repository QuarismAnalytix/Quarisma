#pragma once

#include <cstdint>
#include <functional>

#include "common/macros.h"
#include "smp/quarisma_thread_local.h"
#include "smp_new/parallel/parallelize_1d.h"

namespace quarisma::smp_new::parallel
{

/**
 * @brief Parallel iteration over a range with lambda support.
 *
 * Executes a function in parallel over the range [begin, end).
 * The range is divided into chunks of size grain_size, and each chunk
 * is executed by a different thread.
 *
 * @tparam Functor Lambda or function object type
 * @param begin Start index (inclusive)
 * @param end End index (exclusive)
 * @param grain_size Minimum work per thread. If <= 0, automatically determined.
 * @param f Lambda: void(int64_t begin, int64_t end)
 *
 * @note Thread-safe. Can be called from multiple threads.
 * @note Nested parallelism is supported but may be limited by thread pool size.
 */
template <typename Functor>
void parallel_for(int64_t begin, int64_t end, int64_t grain_size, const Functor& f);

/**
 * @brief Parallel reduction operation.
 *
 * Executes a reduction operation in parallel across multiple threads,
 * combining partial results with a combine function.
 *
 * @tparam T Result type
 * @tparam ReduceFunctor Reduction function type
 * @tparam CombineFunctor Combine function type
 *
 * @param begin Start index (inclusive)
 * @param end End index (exclusive)
 * @param grain_size Minimum work per thread. If <= 0, automatically determined.
 * @param identity Initial value for reduction
 * @param reduce_fn Function: T(int64_t begin, int64_t end, T ident) -> T
 * @param combine_fn Function: T(T a, T b) -> T
 *
 * @return Combined result from all threads
 *
 * @note Thread-safe. Can be called from multiple threads.
 * @note Each thread gets its own partial result initialized with identity.
 */
template <typename T, typename ReduceFunctor, typename CombineFunctor>
T parallel_reduce(
    int64_t               begin,
    int64_t               end,
    int64_t               grain_size,
    const T&              identity,
    const ReduceFunctor&  reduce_fn,
    const CombineFunctor& combine_fn);

/**
 * @brief Launches a task for inter-op parallelism.
 *
 * Executes a task asynchronously on the inter-op thread pool.
 * This is used for parallelism between independent operations.
 *
 * @param fn Task to execute
 *
 * @note Thread-safe.
 * @note The function returns immediately without waiting for the task to complete.
 */
QUARISMA_API void launch(std::function<void()> fn);

/**
 * @brief Launches a task for intra-op parallelism.
 *
 * Executes a task asynchronously on the intra-op thread pool.
 * This is used for parallelism within a single operation.
 *
 * If nested or pool exhausted, executes inline.
 *
 * @param fn Task to execute
 *
 * @note Thread-safe.
 * @note The function returns immediately without waiting for the task to complete.
 */
QUARISMA_API void intraop_launch(std::function<void()> fn);

/**
 * @brief Set the parallel backend.
 *
 * Selects which threading backend to use for parallel operations.
 * Must be called before any parallel work is started.
 *
 * @param backend The backend type (NATIVE, OPENMP, or AUTO)
 *
 * @note Thread-safe.
 * @throws std::runtime_error if the requested backend is not available.
 */
QUARISMA_API void set_backend(int backend);

/**
 * @brief Get the current parallel backend.
 *
 * @return The currently active backend type (0=NATIVE, 1=OPENMP, 2=AUTO)
 */
QUARISMA_API int get_backend();

/**
 * @brief Check if OpenMP backend is available.
 *
 * @return true if OpenMP support is compiled in, false otherwise.
 */
QUARISMA_API bool is_openmp_available();

/**
 * @brief Check if TBB backend is available.
 *
 * @return true if TBB support is compiled in, false otherwise.
 */
QUARISMA_API bool is_tbb_available();

/**
 * @brief Sets the number of threads for intra-op parallelism.
 *
 * This function must be called before any parallel work is started.
 * Once parallel work has started, the number of threads cannot be changed.
 *
 * @param nthreads The number of threads to use. Must be positive.
 *
 * @note Thread-safe. Uses atomic compare-and-swap to ensure thread safety.
 */
QUARISMA_API void set_num_intraop_threads(int nthreads);

/**
 * @brief Gets the number of threads for intra-op parallelism.
 *
 * If the number of threads has not been explicitly set, returns the default
 * number of threads based on the system's hardware concurrency.
 *
 * @return The number of threads in the intra-op thread pool.
 */
QUARISMA_API size_t get_num_intraop_threads();

/**
 * @brief Sets the number of threads for inter-op parallelism.
 *
 * This function must be called before any parallel work is started.
 * Once parallel work has started, the number of threads cannot be changed.
 *
 * @param nthreads The number of threads to use. Must be positive.
 *
 * @note Thread-safe. Uses atomic compare-and-swap to ensure thread safety.
 */
QUARISMA_API void set_num_interop_threads(int nthreads);

/**
 * @brief Gets the number of threads for inter-op parallelism.
 *
 * If the number of threads has not been explicitly set, returns the default
 * number of threads based on the system's hardware concurrency.
 *
 * @return The number of threads in the inter-op thread pool.
 */
QUARISMA_API size_t get_num_interop_threads();

/**
 * @brief Gets the current thread number in a parallel region.
 *
 * Returns the thread ID (0-based) of the current thread if executing
 * in a parallel region, or 0 if not in a parallel region.
 *
 * @return The thread ID (0-based) or 0 if not in parallel region.
 *
 * @note Thread-safe. Can be called from any thread.
 */
QUARISMA_API int get_thread_num();

/**
 * @brief Checks if the current thread is in a parallel region.
 *
 * Returns true if the current thread is executing within a parallel
 * operation (parallel_for, parallel_reduce, etc.), false otherwise.
 *
 * @return true if in a parallel region, false otherwise.
 *
 * @note Thread-safe. Can be called from any thread.
 */
QUARISMA_API bool in_parallel_region();

/**
 * @brief Gets detailed parallelization settings information.
 *
 * Returns a string containing information about the current parallelization
 * configuration, including backend type, thread counts, and other settings.
 *
 * @return A string describing the parallelization settings.
 *
 * @note Thread-safe. Can be called from any thread.
 */
QUARISMA_API std::string get_parallel_info();

}  // namespace quarisma::smp_new::parallel

// ============================================================================
// Template Implementations
// ============================================================================

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <vector>

#include "smp_new/core/thread_pool.h"
#include "smp_new/native/parallel_native.h"
#include "smp_new/tbb/parallel_tbb.h"

namespace quarisma::smp_new::parallel
{

// Forward declarations for internal functions
namespace internal
{
QUARISMA_API void set_thread_num(int thread_id);
QUARISMA_API void set_in_parallel_region(bool in_region);
QUARISMA_API core::TaskThreadPoolBase& GetInteropPool();
QUARISMA_API core::TaskThreadPoolBase& GetIntraopPool();
}  // namespace internal

template <typename Functor>
void parallel_for(int64_t begin, int64_t end, int64_t grain_size, const Functor& f)
{
    if (begin >= end)
    {
        return;
    }

    // Route to appropriate backend based on current selection
    native::BackendType backend = native::GetCurrentBackend();
    if (backend == native::BackendType::TBB)
    {
        // Convert functor to std::function and delegate to TBB backend
        std::function<void(int64_t, int64_t)> func = f;
        tbb::ParallelForTBB(begin, end, grain_size, func);
        return;
    }
    // For NATIVE and OPENMP backends, use the native implementation below

    int64_t n = end - begin;

    // Determine grain size
    if (grain_size <= 0)
    {
        auto num_threads = static_cast<int64_t>(internal::GetInteropPool().Size());
        grain_size       = std::max(static_cast<int64_t>(1), n / (num_threads * 4));
    }

    // If work is small enough, execute serially
    if (grain_size >= n)
    {
        // Save previous parallel region state for proper restoration in nested calls
        bool prev_in_parallel = in_parallel_region();
        int  prev_thread_id   = get_thread_num();
        internal::set_in_parallel_region(true);
        internal::set_thread_num(0);
        try
        {
            f(begin, end);
        }
        catch (...)
        {
            internal::set_in_parallel_region(prev_in_parallel);
            internal::set_thread_num(prev_thread_id);
            throw;
        }
        internal::set_in_parallel_region(prev_in_parallel);
        internal::set_thread_num(prev_thread_id);
        return;
    }

    // Parallel execution with local barrier
    auto&                   pool = internal::GetIntraopPool();
    std::atomic<int64_t>    tasks_completed{0};
    std::atomic<int64_t>    task_counter{0};
    std::atomic<bool>       exception_occurred{false};
    std::exception_ptr      captured_exception;
    std::mutex              exception_mutex;
    std::condition_variable barrier_cv;
    std::mutex              barrier_mutex;

    // Count total number of tasks
    int64_t num_tasks = (n + grain_size - 1) / grain_size;

    for (int64_t i = begin; i < end; i += grain_size)
    {
        int64_t chunk_end = std::min(i + grain_size, end);
        int64_t task_id   = task_counter.fetch_add(1, std::memory_order_relaxed);

        pool.Run(
            [&f,
             i,
             chunk_end,
             task_id,
             &tasks_completed,
             &exception_occurred,
             &captured_exception,
             &exception_mutex,
             &barrier_cv,
             &barrier_mutex,
             num_tasks]()
            {
                // Save previous parallel region state for proper restoration
                bool prev_in_parallel = in_parallel_region();
                int  prev_thread_id   = get_thread_num();
                internal::set_in_parallel_region(true);
                const int thread_slot =
                    static_cast<int>(std::min<int64_t>(task_id, std::numeric_limits<int>::max()));
                internal::set_thread_num(thread_slot);
                try
                {
                    f(i, chunk_end);
                }
                catch (...)
                {
                    exception_occurred.store(true);
                    {
                        std::lock_guard<std::mutex> lock(exception_mutex);
                        if (!captured_exception)
                        {
                            captured_exception = std::current_exception();
                        }
                    }
                }
                internal::set_in_parallel_region(prev_in_parallel);
                internal::set_thread_num(prev_thread_id);

                // Increment completion counter and notify barrier
                int64_t completed = tasks_completed.fetch_add(1, std::memory_order_acq_rel) + 1;
                if (completed == num_tasks)
                {
                    std::lock_guard<std::mutex> lock(barrier_mutex);
                    barrier_cv.notify_all();
                }
            });
    }

    // Wait for all tasks to complete using local barrier
    {
        std::unique_lock<std::mutex> lock(barrier_mutex);
        barrier_cv.wait(
            lock,
            [&tasks_completed, num_tasks]()
            { return tasks_completed.load(std::memory_order_acquire) == num_tasks; });
    }

    // Rethrow any captured exception
    if (exception_occurred.load() && captured_exception)
    {
        std::rethrow_exception(captured_exception);
    }
}

template <typename T, typename ReduceFunctor, typename CombineFunctor>
T parallel_reduce(
    int64_t               begin,
    int64_t               end,
    int64_t               grain_size,
    const T&              identity,
    const ReduceFunctor&  reduce_fn,
    const CombineFunctor& combine_fn)
{
    if (begin >= end)
    {
        return identity;
    }

    // Route to appropriate backend based on current selection
    native::BackendType backend = native::GetCurrentBackend();
    if (backend == native::BackendType::TBB)
    {
        // Convert functors to std::function and delegate to TBB backend
        std::function<T(int64_t, int64_t, T)> reduce_func  = reduce_fn;
        std::function<T(T, T)>                combine_func = combine_fn;
        return tbb::ParallelReduceTBB(begin, end, grain_size, identity, reduce_func, combine_func);
    }
    // For NATIVE and OPENMP backends, use the native implementation below

    int64_t n = end - begin;

    // Determine grain size
    if (grain_size <= 0)
    {
        auto num_threads = static_cast<int64_t>(internal::GetInteropPool().Size());
        grain_size       = std::max(static_cast<int64_t>(1), n / (num_threads * 4));
    }

    // If work is small enough, execute serially
    if (grain_size >= n)
    {
        // Save previous parallel region state for proper restoration in nested calls
        bool prev_in_parallel = in_parallel_region();
        int  prev_thread_id   = get_thread_num();
        internal::set_in_parallel_region(true);
        internal::set_thread_num(0);
        T result{};
        try
        {
            result = reduce_fn(begin, end, identity);
        }
        catch (...)
        {
            internal::set_in_parallel_region(prev_in_parallel);
            internal::set_thread_num(prev_thread_id);
            throw;
        }
        internal::set_in_parallel_region(prev_in_parallel);
        internal::set_thread_num(prev_thread_id);
        return result;
    }

    // Calculate number of chunks
    int64_t        num_chunks = (n + grain_size - 1) / grain_size;
    std::vector<T> partial_results(num_chunks, identity);

    // Parallel reduction with local barrier
    auto&                   pool = internal::GetIntraopPool();
    std::atomic<int64_t>    tasks_completed{0};
    std::atomic<int64_t>    task_counter{0};
    std::atomic<bool>       exception_occurred{false};
    std::exception_ptr      captured_exception;
    std::mutex              exception_mutex;
    std::condition_variable barrier_cv;
    std::mutex              barrier_mutex;

    for (int64_t i = begin; i < end; i += grain_size)
    {
        int64_t chunk_end = std::min(i + grain_size, end);
        int64_t task_id   = task_counter.fetch_add(1, std::memory_order_relaxed);

        pool.Run(
            [&reduce_fn,
             &partial_results,
             i,
             chunk_end,
             &identity,
             task_id,
             &tasks_completed,
             &exception_occurred,
             &captured_exception,
             &exception_mutex,
             &barrier_cv,
             &barrier_mutex,
             num_chunks]()
            {
                // Save previous parallel region state for proper restoration
                bool prev_in_parallel = in_parallel_region();
                int  prev_thread_id   = get_thread_num();
                internal::set_in_parallel_region(true);
                const int thread_slot =
                    static_cast<int>(std::min<int64_t>(task_id, std::numeric_limits<int>::max()));
                internal::set_thread_num(thread_slot);
                try
                {
                    partial_results[static_cast<size_t>(task_id)] =
                        reduce_fn(i, chunk_end, identity);
                }
                catch (...)
                {
                    exception_occurred.store(true);
                    {
                        std::lock_guard<std::mutex> lock(exception_mutex);
                        if (!captured_exception)
                        {
                            captured_exception = std::current_exception();
                        }
                    }
                }
                internal::set_in_parallel_region(prev_in_parallel);
                internal::set_thread_num(prev_thread_id);

                // Increment completion counter and notify barrier
                int64_t completed = tasks_completed.fetch_add(1, std::memory_order_acq_rel) + 1;
                if (completed == num_chunks)
                {
                    std::lock_guard<std::mutex> lock(barrier_mutex);
                    barrier_cv.notify_all();
                }
            });
    }

    // Wait for all tasks to complete using local barrier
    {
        std::unique_lock<std::mutex> lock(barrier_mutex);
        barrier_cv.wait(
            lock,
            [&tasks_completed, num_chunks]()
            { return tasks_completed.load(std::memory_order_acquire) == num_chunks; });
    }

    // Rethrow any captured exception
    if (exception_occurred.load() && captured_exception)
    {
        std::rethrow_exception(captured_exception);
    }

    // Combine partial results
    T result = partial_results[0];
    for (int64_t i = 1; i < num_chunks; ++i)
    {
        result = combine_fn(result, partial_results[static_cast<size_t>(i)]);
    }
    return result;
}

}  // namespace quarisma::smp_new::parallel
