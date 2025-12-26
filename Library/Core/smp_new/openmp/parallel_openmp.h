#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "common/macros.h"

namespace quarisma::smp_new::openmp
{

/**
 * @brief OpenMP parallel execution backend.
 *
 * This module provides OpenMP-based parallel execution for Quarisma's
 * parallel APIs, matching PyTorch's OpenMP backend.
 *
 * Features:
 * - Directive-based parallelism (#pragma omp)
 * - Integration with MKL and Intel OpenMP
 * - Automatic thread pool management
 * - Environment variable configuration (OMP_NUM_THREADS, MKL_NUM_THREADS)
 * - Nested parallelism support
 * - Exception handling
 *
 * Requirements:
 * - OpenMP support at compile time
 * - Optional: Intel MKL for optimized performance
 */

/**
 * @brief Initialize the OpenMP parallel backend.
 *
 * This function initializes OpenMP for parallel execution.
 * Requires OpenMP support at compile time.
 *
 * @note Thread-safe. Can be called multiple times.
 * @throws std::runtime_error if OpenMP is not available.
 */
QUARISMA_API void InitializeOpenMPBackend();

/**
 * @brief Shutdown the OpenMP parallel backend.
 *
 * This function shuts down OpenMP resources.
 *
 * @note Thread-safe. Can be called multiple times.
 */
QUARISMA_API void ShutdownOpenMPBackend();

/**
 * @brief Check if the OpenMP backend is initialized.
 *
 * @return true if the OpenMP backend is initialized, false otherwise.
 */
QUARISMA_API bool IsOpenMPBackendInitialized();

/**
 * @brief Check if OpenMP is available.
 *
 * @return true if OpenMP support is compiled in, false otherwise.
 */
QUARISMA_API bool IsOpenMPAvailable();

/**
 * @brief Set the number of OpenMP threads.
 *
 * This function sets the number of threads for OpenMP parallelism.
 * Also configures MKL if available.
 *
 * @param nthreads The number of threads to use. Must be positive.
 *
 * @note Thread-safe.
 * @throws std::runtime_error if OpenMP is not available.
 */
QUARISMA_API void SetNumOpenMPThreads(int nthreads);

/**
 * @brief Get the number of OpenMP threads.
 *
 * @return The number of OpenMP threads.
 *
 * @throws std::runtime_error if OpenMP is not available.
 */
QUARISMA_API int GetNumOpenMPThreads();

/**
 * @brief Get the current OpenMP thread ID.
 *
 * @return The thread ID (0-based) or 0 if not in parallel region.
 */
QUARISMA_API int GetOpenMPThreadNum();

/**
 * @brief Check if currently in an OpenMP parallel region.
 *
 * @return true if in parallel region, false otherwise.
 */
QUARISMA_API bool InOpenMPParallelRegion();

/**
 * @brief Get information about the OpenMP backend.
 *
 * @return A string containing OpenMP backend information (version, configuration, etc.)
 */
QUARISMA_API std::string GetOpenMPBackendInfo();

}  // namespace quarisma::smp_new::openmp
