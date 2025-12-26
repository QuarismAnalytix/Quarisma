#pragma once

#include <cstddef>
#include <functional>

#include "common/macros.h"

namespace quarisma::detail::smp::Advanced
{

/**
 * @brief Sets the number of threads for interop thread pool.
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
 * @brief Gets the number of threads in the interop thread pool.
 *
 * If the number of threads has not been explicitly set, returns the default
 * number of threads based on the system's hardware concurrency.
 *
 * @return The number of threads in the interop thread pool.
 */
QUARISMA_API size_t get_num_interop_threads();

/**
 * @brief Launches a function on the interop thread pool without thread state.
 *
 * This function queues the provided function to be executed on one of the
 * available threads in the interop thread pool. The function is executed
 * asynchronously.
 *
 * @param fn The function to execute. Must be a valid callable.
 *
 * @note Thread-safe. The function is queued and executed by a worker thread.
 */
QUARISMA_API void launch_no_thread_state(std::function<void()> fn);

/**
 * @brief Launches a function on the interop thread pool.
 *
 * This function queues the provided function to be executed on one of the
 * available threads in the interop thread pool. The function is executed
 * asynchronously.
 *
 * @param fn The function to execute. Must be a valid callable.
 *
 * @note Thread-safe. The function is queued and executed by a worker thread.
 */
QUARISMA_API void launch(std::function<void()> fn);

}  // namespace quarisma::detail::smp::Advanced
