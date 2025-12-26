#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "common/macros.h"

namespace quarisma::smp_new::native
{

/**
 * @brief Parallel execution backend types.
 *
 * Supported backends:
 * - NATIVE: std::thread-based thread pool (default)
 * - OPENMP: OpenMP-based parallelism
 * - AUTO: Automatically select best available backend
 * - TBB: Intel Threading Building Blocks
 */
enum class BackendType
{
    NATIVE = 0,  ///< Native std::thread backend
    OPENMP = 1,  ///< OpenMP backend
    AUTO   = 2,  ///< Auto-select best available
    TBB    = 3   ///< Intel TBB backend
};

/**
 * @brief Native CPU parallel execution backend.
 *
 * This module provides multiple CPU threading implementations for Quarisma's
 * parallel APIs, matching PyTorch's threading architecture.
 *
 * Supported Backends:
 * - Native (std::thread): Master-worker pattern with lazy initialization
 * - OpenMP: Directive-based parallelism with MKL integration
 * - TBB: Intel Threading Building Blocks with task-based parallelism
 *
 * Features:
 * - Master-worker pattern for reduced overhead
 * - Lazy thread pool initialization
 * - Separate intra-op and inter-op thread pools
 * - Work-stealing for load balancing
 * - Robust exception handling
 * - NUMA support
 * - Backend selection at runtime or compile-time
 */

/**
 * @brief Initialize the parallel backend.
 *
 * This function initializes the thread pools and other resources needed
 * for parallel execution. It is called automatically on first use.
 *
 * @param backend The backend type to initialize. If AUTO, selects the best available.
 *
 * @note Thread-safe. Can be called multiple times.
 */
QUARISMA_API void InitializeBackend(BackendType backend = BackendType::AUTO);

/**
 * @brief Initialize the native parallel backend (std::thread).
 *
 * This function initializes the thread pools and other resources needed
 * for parallel execution using std::thread. It is called automatically on first use.
 *
 * @note Thread-safe. Can be called multiple times.
 */
QUARISMA_API void InitializeNativeBackend();

/**
 * @brief Shutdown the parallel backend.
 *
 * This function shuts down the thread pools and releases resources.
 * After calling this function, the backend must be reinitialized before
 * using parallel APIs.
 *
 * @note Thread-safe. Can be called multiple times.
 */
QUARISMA_API void ShutdownBackend();

/**
 * @brief Shutdown the native parallel backend.
 *
 * This function shuts down the thread pools and releases resources.
 * After calling this function, the backend must be reinitialized before
 * using parallel APIs.
 *
 * @note Thread-safe. Can be called multiple times.
 */
QUARISMA_API void ShutdownNativeBackend();

/**
 * @brief Check if the backend is initialized.
 *
 * @return true if the backend is initialized, false otherwise.
 */
QUARISMA_API bool IsBackendInitialized();

/**
 * @brief Check if the native backend is initialized.
 *
 * @return true if the native backend is initialized, false otherwise.
 */
QUARISMA_API bool IsNativeBackendInitialized();

/**
 * @brief Get the current backend type.
 *
 * @return The currently active backend type.
 */
QUARISMA_API BackendType GetCurrentBackend();

/**
 * @brief Get information about the parallel backend.
 *
 * @return A string containing backend information (type, version, configuration, etc.)
 */
QUARISMA_API std::string GetBackendInfo();

/**
 * @brief Get information about the native backend.
 *
 * @return A string containing native backend information (version, configuration, etc.)
 */
QUARISMA_API std::string GetNativeBackendInfo();

}  // namespace quarisma::smp_new::native
