/*
 * Quarisma Profiler RAII Guards
 *
 * RAII wrappers for automatic profiler lifecycle management.
 * Ensures profiler is properly started and stopped.
 *
 * Features:
 * - Automatic profiler start on construction
 * - Automatic profiler stop on destruction
 * - Exception-safe operation
 * - Scope-based function recording
 *
 * Usage:
 *   {
 *       quarisma::profiler::ProfilerConfig config;
 *       config.activities = {quarisma::profiler::ActivityType::CPU};
 *       quarisma::profiler::ProfilerGuard guard(config);
 *       // Profiling active here
 *       // ... code to profile ...
 *   } // Profiling stops automatically
 */

#pragma once

#ifndef QUARISMA_PROFILER_GUARD_H
#define QUARISMA_PROFILER_GUARD_H

#include <string>

#include "../common/export.h"
#include "profiler_api.h"

namespace quarisma
{
namespace profiler
{

// ============================================================================
// == Profiler Guard (RAII) ===================================================
// ============================================================================

/**
 * @brief RAII guard for automatic profiler lifecycle management
 *
 * Starts profiler on construction, stops on destruction.
 * Ensures proper cleanup even if exceptions occur.
 */
class QUARISMA_API ProfilerGuard
{
public:
    /**
   * @brief Construct and start profiler
   *
   * @param config Profiler configuration
   */
    explicit ProfilerGuard(const ProfilerConfig& config);

    /**
   * @brief Destruct and stop profiler
   */
    ~ProfilerGuard();

    /**
   * @brief Check if profiler is active
   *
   * @return true if profiler started successfully
   */
    bool is_active() const { return was_profiling_; }

    /**
   * @brief Export trace to file
   *
   * @param path Output file path
   * @return true if export successful
   */
    static bool export_trace(const std::string& path);

private:
    bool           was_profiling_ = false;
    ProfilerConfig config_;
};

// ============================================================================
// == Record Function (Scope-based Recording) ================================
// ============================================================================

/**
 * @brief RAII guard for recording a function/scope
 *
 * Records entry and exit of a function or code block.
 * Automatically annotates with ITT if available.
 *
 * Usage:
 *   void my_function() {
 *       quarisma::profiler::RecordFunction record("my_function");
 *       // ... function code ...
 *   } // Automatically recorded
 */
class QUARISMA_API RecordFunction
{
public:
    /**
   * @brief Construct and record function entry
   *
   * @param name Function name (must be string literal or persistent)
   */
    explicit RecordFunction(const char* name);

    /**
   * @brief Destruct and record function exit
   */
    ~RecordFunction();

    /**
   * @brief Get function name
   *
   * @return Function name
   */
    const char* name() const { return name_; }

    /**
   * @brief Get start time in nanoseconds
   *
   * @return Start time
   */
    uint64_t start_time_ns() const { return start_time_ns_; }

    /**
   * @brief Get end time in nanoseconds
   *
   * @return End time (0 if still recording)
   */
    uint64_t end_time_ns() const { return end_time_ns_; }

private:
    const char* name_;
    uint64_t    start_time_ns_;
    uint64_t    end_time_ns_ = 0;
};

// ============================================================================
// == Scoped Activity Recording ===============================================
// ============================================================================

/**
 * @brief RAII guard for recording a named activity
 *
 * Records a named activity with automatic start/stop.
 * Integrates with both Kineto and ITT.
 *
 * Usage:
 *   {
 *       quarisma::profiler::ScopedActivity activity("matrix_multiply");
 *       // ... code to profile ...
 *   } // Activity automatically recorded
 */
class QUARISMA_API ScopedActivity
{
public:
    /**
   * @brief Construct and start activity
   *
   * @param name Activity name
   */
    explicit ScopedActivity(const char* name);

    /**
   * @brief Destruct and stop activity
   */
    ~ScopedActivity();

    /**
   * @brief Get activity name
   *
   * @return Activity name
   */
    const char* name() const { return name_; }

private:
    const char* name_;
};

}  // namespace profiler
}  // namespace quarisma

#endif  // QUARISMA_PROFILER_GUARD_H
