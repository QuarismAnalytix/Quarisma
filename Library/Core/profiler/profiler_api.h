/*
 * Quarisma Profiler API
 *
 * High-level profiler API matching PyTorch's profiler interface.
 * Provides CPU and GPU profiling with event collection and export.
 *
 * Features:
 * - CPU operation profiling via callbacks
 * - GPU profiling via Kineto (NVIDIA, AMD, Intel XPU)
 * - Memory profiling
 * - Event tree building
 * - JSON export
 * - Thread-safe operation
 *
 * Usage:
 *   quarisma::profiler::ProfilerConfig config;
 *   config.activities = {quarisma::profiler::ActivityType::CPU};
 *   config.output_file = "trace.json";
 *
 *   auto& profiler = quarisma::profiler::ProfilerSession::instance();
 *   profiler.start(config);
 *   // ... code to profile ...
 *   profiler.stop();
 *   profiler.export_trace(config.output_file);
 */

#pragma once

#ifndef QUARISMA_PROFILER_API_H
#define QUARISMA_PROFILER_API_H

#include <cstdint>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "../common/export.h"

namespace quarisma
{
namespace profiler
{

// ============================================================================
// == Activity Types ==========================================================
// ============================================================================

/**
 * @brief Types of activities that can be profiled
 */
enum class ActivityType
{
    CPU    = 0,  ///< CPU operations
    CUDA   = 1,  ///< NVIDIA CUDA operations
    ROCM   = 2,  ///< AMD ROCm operations
    XPU    = 3,  ///< Intel XPU operations
    Memory = 4,  ///< Memory allocations/deallocations
};

// ============================================================================
// == Profiler State ==========================================================
// ============================================================================

/**
 * @brief Profiler operational state
 */
enum class ProfilerState
{
    Disabled  = 0,  ///< Profiler not running
    Ready     = 1,  ///< Profiler configured, ready to start
    Recording = 2,  ///< Profiler actively recording
};

// ============================================================================
// == Profiler Configuration ==================================================
// ============================================================================

/**
 * @brief Configuration for profiler session
 */
struct ProfilerConfig
{
    /// Activities to profile
    std::set<ActivityType> activities;

    /// Record tensor shapes
    bool record_shapes = false;

    /// Profile memory allocations
    bool profile_memory = false;

    /// Capture call stacks
    bool with_stack = false;

    /// Output file path for trace
    std::string output_file;

    /// Trace identifier
    std::string trace_id;

    /// Enable verbose logging
    bool verbose = false;
};

// ============================================================================
// == Profiler Session ========================================================
// ============================================================================

/**
 * @brief Main profiler session manager
 *
 * Singleton class managing profiler lifecycle and event collection.
 * Thread-safe for concurrent profiling from multiple threads.
 */
class QUARISMA_API ProfilerSession
{
public:
    /**
   * @brief Get singleton instance
   */
    static ProfilerSession& instance();

    /**
   * @brief Start profiling with given configuration
   *
   * @param config Profiler configuration
   * @return true if profiling started successfully
   */
    bool start(const ProfilerConfig& config);

    /**
   * @brief Stop profiling and collect events
   *
   * @return true if profiling stopped successfully
   */
    bool stop();

    /**
   * @brief Check if profiler is currently recording
   *
   * @return true if profiling is active
   */
    bool is_profiling() const;

    /**
   * @brief Get current profiler state
   *
   * @return Current ProfilerState
   */
    ProfilerState get_state() const;

    /**
   * @brief Get current configuration
   *
   * @return Current ProfilerConfig
   */
    const ProfilerConfig& get_config() const;

    /**
   * @brief Export collected trace to file
   *
   * @param path Output file path
   * @return true if export successful
   */
    bool export_trace(const std::string& path);

    /**
   * @brief Clear collected events
   */
    void clear();

    /**
   * @brief Get number of collected events
   *
   * @return Number of events
   */
    size_t event_count() const;

    /**
   * @brief Reset profiler to Disabled state (for testing)
   */
    void reset();

private:
    ProfilerSession()  = default;
    ~ProfilerSession() = default;

    // Prevent copying
    ProfilerSession(const ProfilerSession&)            = delete;
    ProfilerSession& operator=(const ProfilerSession&) = delete;

    // State management
    mutable std::mutex state_mutex_;
    ProfilerState      state_ = ProfilerState::Disabled;
    ProfilerConfig     config_;

// Event collection
// Note: C4251 warning suppressed for private member - not part of public interface
#pragma warning(suppress : 4251)
    std::vector<std::string> events_;  // Simplified event storage
    uint64_t                 start_time_ns_ = 0;
    uint64_t                 end_time_ns_   = 0;

    // Helper methods
    void        initialize_kineto() const;
    static void initialize_itt();
    static void finalize_kineto();
    void        finalize_itt();
    void        collect_events() const;
};

// ============================================================================
// == Global API Functions ====================================================
// ============================================================================

/**
 * @brief Check if profiler is enabled
 *
 * @return true if profiler is available and enabled
 */
QUARISMA_API bool profiler_enabled();

/**
 * @brief Get current profiler state
 *
 * @return Current ProfilerState
 */
QUARISMA_API ProfilerState get_profiler_state();

/**
 * @brief Get current profiler configuration
 *
 * @return Current ProfilerConfig
 */
QUARISMA_API const ProfilerConfig& get_profiler_config();

}  // namespace profiler
}  // namespace quarisma

#endif  // QUARISMA_PROFILER_API_H
