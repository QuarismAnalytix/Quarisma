/*
 * Quarisma Kineto Shim - Direct libkineto Integration
 *
 * This header provides direct access to PyTorch Kineto profiling library,
 * aligned with PyTorch's implementation for feature parity.
 *
 * Features:
 * - Direct libkineto::api() usage
 * - Support for CPU and GPU activity tracing
 * - Configurable activity types
 * - Thread-safe profiling session management
 *
 * Usage:
 *   libkineto_init(false, true);  // GPU, logOnError
 *   libkineto::api().activityProfiler().prepareTrace(activities);
 *   libkineto::api().activityProfiler().startTrace();
 *   // ... code to profile ...
 *   auto trace = libkineto::api().activityProfiler().stopTrace();
 *   trace->save("trace.json");
 *
 * Note: This file is only compiled when QUARISMA_HAS_KINETO=ON.
 * Build-time exclusion is handled in CMakeLists.txt.
 */

#pragma once

#include <memory>
#include <set>
#include <string>

#include "common/export.h"
#include "common/macros.h"

#if QUARISMA_HAS_KINETO
#include <ActivityType.h>
#endif

// Forward declarations to avoid including libkineto.h in headers
namespace libkineto
{
class GenericTraceActivity;
struct CpuTraceBuffer;
class ActivityTraceInterface;
class ActivityProfiler;
}  // namespace libkineto

namespace quarisma
{
namespace profiler
{

// Custom deleter for void pointers (used in stub implementation)
struct void_deleter
{
    void operator()(void*) const noexcept {}
};

constexpr bool kKinetoAvailable{true};

// ============================================================================
// Kineto Initialization and Configuration
// ============================================================================

/**
 * @brief Initialize Kineto profiling library
 *
 * This function initializes libkineto with support for the specified backends.
 * Should be called once before any profiling operations.
 *
 * @param cpu_only If true, only CPU profiling is enabled
 * @param log_on_error If true, log errors during initialization
 */
QUARISMA_API void kineto_init(bool cpu_only = false, bool log_on_error = true);

/**
 * @brief Check if Kineto profiler is registered
 *
 * @return true if profiler is registered, false otherwise
 */
QUARISMA_API bool kineto_is_profiler_registered();

/**
 * @brief Check if Kineto profiler is initialized
 *
 * @return true if profiler is initialized, false otherwise
 */
QUARISMA_API bool kineto_is_profiler_initialized();

/**
 * @brief Prepare trace with specified activity types
 *
 * @param activities Set of activity types to trace
 * @param config_str Optional configuration string
 */
QUARISMA_API void kineto_prepare_trace(
    const std::set<libkineto::ActivityType>& activities, const std::string& config_str = "");

/**
 * @brief Start profiling trace
 */
QUARISMA_API void kineto_start_trace();

/**
 * @brief Stop profiling trace and return trace interface
 *
 * @return Void pointer to ActivityTraceInterface (caller must cast to libkineto::ActivityTraceInterface*)
 */
QUARISMA_API void* kineto_stop_trace();

/**
 * @brief Reset Kineto thread-local state
 */
QUARISMA_API void kineto_reset_tls();

}  // namespace profiler
}  // namespace quarisma
