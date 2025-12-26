

// This file incorporates code from the Visualization Toolkit (VTK) and remains subject to the BSD-3-Clause VTK license.

#include "smp/tools.h"

#include "smp/Common/tools_api.h"  // for tools_api

//#include "xsigmaSMP.h"

//------------------------------------------------------------------------------
namespace xsigma
{
/**
 * @brief Gets the current backend implementation being used
 *
 * Returns the name of the current backend that handles parallel processing
 * (e.g., "STDThread" or "TBB").
 *
 * @return Name of the current backend as a C-string
 */
const char* tools::GetBackend()
{
    auto& SMPToolsAPI = xsigma::detail::smp::tools_api::GetInstance();
    return SMPToolsAPI.GetBackend();
}

//------------------------------------------------------------------------------
/**
 * @brief Sets the backend implementation to use for parallel processing
 *
 * Attempts to switch the parallel processing backend to the specified
 * implementation. Valid backends include "STDThread" and "TBB".
 * The availability of backends depends on compile-time configuration.
 *
 * @param backend Name of the backend to use
 * @return true if backend was successfully set, false otherwise
 */
bool tools::SetBackend(const char* backend)
{
    auto& SMPToolsAPI = xsigma::detail::smp::tools_api::GetInstance();
    return SMPToolsAPI.SetBackend(backend);
}

//------------------------------------------------------------------------------
/**
 * @brief Initializes the parallel processing subsystem
 *
 * Sets up the parallel processing backend with the specified number of threads.
 * If numThreads is 0, the system will use a default value based on the
 * available hardware concurrency.
 *
 * @param numThreads Number of threads to use (0 for system default)
 */
void tools::Initialize(int numThreads)
{
    auto& SMPToolsAPI = xsigma::detail::smp::tools_api::GetInstance();
    return SMPToolsAPI.Initialize(numThreads);
}

//------------------------------------------------------------------------------
/**
 * @brief Gets the estimated number of threads based on current configuration
 *
 * Returns the number of threads that would be used for parallel processing
 * based on the current configuration, which may be different from the
 * number of physical cores available.
 *
 * @return Estimated number of threads
 */
int tools::GetEstimatedNumberOfThreads()
{
    auto& SMPToolsAPI = xsigma::detail::smp::tools_api::GetInstance();
    return SMPToolsAPI.GetEstimatedNumberOfThreads();
}

//------------------------------------------------------------------------------
/**
 * @brief Gets the default number of threads for the system
 *
 * Returns the system's default thread count, typically based on the available
 * hardware concurrency (number of logical cores). This is the value used
 * when Initialize is called with numThreads=0.
 *
 * @return Default number of threads
 */
int tools::GetEstimatedDefaultNumberOfThreads()
{
    auto& SMPToolsAPI = xsigma::detail::smp::tools_api::GetInstance();
    return SMPToolsAPI.GetEstimatedDefaultNumberOfThreads();
}

//------------------------------------------------------------------------------
/**
 * @brief Enables or disables nested parallelism
 *
 * Configures whether nested parallel regions are allowed. When enabled,
 * parallel code inside an already parallel region will create additional
 * threads. When disabled, parallel code inside an already parallel region
 * will run sequentially.
 *
 * @param isNested true to enable nested parallelism, false to disable
 */
void tools::SetNestedParallelism(bool isNested)
{
    auto& SMPToolsAPI = xsigma::detail::smp::tools_api::GetInstance();
    return SMPToolsAPI.SetNestedParallelism(isNested);
}

//------------------------------------------------------------------------------
/**
 * @brief Gets the current nested parallelism setting
 *
 * Returns whether nested parallel regions are currently enabled or disabled.
 *
 * @return true if nested parallelism is enabled, false otherwise
 */
bool tools::GetNestedParallelism()
{
    auto& SMPToolsAPI = xsigma::detail::smp::tools_api::GetInstance();
    return SMPToolsAPI.GetNestedParallelism();
}

//------------------------------------------------------------------------------
/**
 * @brief Checks if the current execution is within a parallel scope
 *
 * Determines whether the calling code is currently executing within
 * a parallel section created by the parallel processing backend.
 *
 * @return true if in a parallel scope, false if in a sequential scope
 */
bool tools::IsParallelScope()
{
    auto& SMPToolsAPI = xsigma::detail::smp::tools_api::GetInstance();
    return SMPToolsAPI.IsParallelScope();
}

//------------------------------------------------------------------------------
/**
 * @brief Checks if single-thread mode is active
 *
 * Returns whether the parallel subsystem is currently running in
 * single-threaded mode. This may occur due to resource constraints
 * or explicit configuration.
 *
 * @return true if running in single-threaded mode, false otherwise
 */
bool tools::GetSingleThread()
{
    auto& SMPToolsAPI = xsigma::detail::smp::tools_api::GetInstance();
    return SMPToolsAPI.GetSingleThread();
}
}  // namespace xsigma
