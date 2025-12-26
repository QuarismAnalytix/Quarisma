/*
 * Quarisma Kineto Profiler Wrapper
 *
 * This header provides a C++ wrapper around PyTorch Kineto profiling library,
 * enabling advanced performance profiling capabilities for Quarisma applications.
 *
 * Features:
 * - Thread-safe profiling session management
 * - Support for CPU and GPU activity tracing
 * - Configurable profiling options
 * - Error handling via return values (no exceptions)
 *
 * Usage:
 *   auto profiler = quarisma::kineto_profiler::create();
 *   if (profiler) {
 *     if (profiler->start_profiling()) {
 *       // ... code to profile ...
 *       profiler->stop_profiling();
 *     }
 *   }
 */

#ifndef QUARISMA_KINETO_PROFILER_H_
#define QUARISMA_KINETO_PROFILER_H_

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "common/export.h"

namespace quarisma
{
namespace kineto_profiler
{

/**
 * @brief Configuration options for Kineto profiling sessions.
 *
 * Controls which activities are traced and how profiling data is collected.
 */
struct profiling_config
{
    /// Enable CPU activity tracing
    bool enable_cpu_tracing = true;

    /// Enable GPU activity tracing (if available)
    bool enable_gpu_tracing = false;

    /// Enable memory profiling
    bool enable_memory_profiling = false;

    /// Output directory for profiling results
    std::string output_dir = "./kineto_profiles";

    /// Trace name/identifier
    std::string trace_name = "quarisma_trace";

    /// Maximum number of activities to collect (0 = unlimited)
    int max_activities = 0;
};

/**
 * @brief Profiling session result information.
 *
 * Contains metadata about a completed profiling session.
 */
struct profiling_result
{
    /// Whether profiling completed successfully
    bool success = false;

    /// Number of activities collected
    int activity_count = 0;

    /// Output file path (if applicable)
    std::string output_file;

    /// Error message (if any)
    std::string error_message;
};

/**
 * @brief Main Kineto profiler wrapper class.
 *
 * Provides a thread-safe interface to PyTorch Kineto profiling functionality.
 * Follows Quarisma conventions: snake_case naming, no exceptions, return-value
 * based error handling.
 */
class QUARISMA_VISIBILITY kineto_profiler
{
public:
    /**
   * @brief Factory method to create a new profiler instance.
   *
   * @return Unique pointer to profiler, or nullptr if creation fails.
   */
    QUARISMA_API static std::unique_ptr<kineto_profiler> create();

    /**
   * @brief Factory method with custom configuration.
   *
   * @param config Profiling configuration options.
   * @return Unique pointer to profiler, or nullptr if creation fails.
   */
    QUARISMA_API static std::unique_ptr<kineto_profiler> create_with_config(
        const profiling_config& config);

    /// Destructor - ensures profiling is stopped and resources are cleaned up.
    QUARISMA_API ~kineto_profiler();

    // Prevent copying
    kineto_profiler(const kineto_profiler&)            = delete;
    kineto_profiler& operator=(const kineto_profiler&) = delete;

    // Prevent moving (mutex is not movable)
    kineto_profiler(kineto_profiler&&)            = delete;
    kineto_profiler& operator=(kineto_profiler&&) = delete;

    /**
   * @brief Start a profiling session.
   *
   * @return true if profiling started successfully, false otherwise.
   */
    QUARISMA_API bool start_profiling();

    /**
   * @brief Stop the current profiling session.
   *
   * @return Result information including activity count and output file path.
   */
    QUARISMA_API profiling_result stop_profiling();

    /**
   * @brief Check if profiling is currently active.
   *
   * @return true if profiling session is running, false otherwise.
   */
    QUARISMA_API bool is_profiling() const;

    /**
   * @brief Get the current profiling configuration.
   *
   * @return Reference to the current configuration.
   */
    QUARISMA_API const profiling_config& get_config() const;

    /**
   * @brief Update profiling configuration.
   *
   * Note: Configuration changes take effect on the next profiling session.
   *
   * @param config New configuration options.
   * @return true if configuration was updated, false if profiling is active.
   */
    QUARISMA_API bool set_config(const profiling_config& config);

    /**
   * @brief Initialize Kineto library (called automatically on first use).
   *
   * @param cpu_only If true, only CPU profiling is enabled.
   * @return true if initialization succeeded, false otherwise.
   */
    QUARISMA_API static bool initialize(bool cpu_only = true);

    /**
   * @brief Check if Kineto library is initialized.
   *
   * @return true if Kineto has been initialized, false otherwise.
   */
    QUARISMA_API static bool is_initialized();

private:
    /// Private constructor - use factory methods instead.
    explicit kineto_profiler(profiling_config config);

    /// Current profiling configuration
    profiling_config config_;

    /// Whether profiling is currently active
    bool is_profiling_ = false;

    /// Mutex for thread-safe access to profiling state
    mutable std::mutex profiling_mutex_;

    /// Static initialization flag
    static bool       initialized_;
    static std::mutex init_mutex_;
};

}  // namespace kineto_profiler
}  // namespace quarisma

#endif  // QUARISMA_KINETO_PROFILER_H_
