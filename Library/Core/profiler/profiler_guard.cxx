/*
 * XSigma Profiler RAII Guards Implementation
 */

#include "profiler_guard.h"

#include <chrono>

#include "itt_wrapper.h"
#include "profiler_api.h"

namespace xsigma
{
namespace profiler
{

// ============================================================================
// == ProfilerGuard Implementation ============================================
// ============================================================================

ProfilerGuard::ProfilerGuard(const ProfilerConfig& config) : config_(config)
{
    auto& profiler = ProfilerSession::instance();
    was_profiling_ = profiler.start(config);
}

ProfilerGuard::~ProfilerGuard()
{
    if (was_profiling_)
    {
        auto& profiler = ProfilerSession::instance();
        profiler.stop();
    }
}

bool ProfilerGuard::export_trace(const std::string& path)
{
    auto& profiler = ProfilerSession::instance();
    return profiler.export_trace(path);
}

// ============================================================================
// == RecordFunction Implementation ===========================================
// ============================================================================

RecordFunction::RecordFunction(const char* name) : name_(name)
{
    // Record start time
    start_time_ns_ = std::chrono::high_resolution_clock::now().time_since_epoch().count();

    // Emit ITT range push if available
#if XSIGMA_HAS_ITT
    itt_range_push(name_);
#endif

    // Check if profiler is active
    if (profiler_enabled())
    {
        // In a full implementation, this would register the function
        // with the active profiler for event collection
    }
}

RecordFunction::~RecordFunction()
{
    // Record end time
    end_time_ns_ = std::chrono::high_resolution_clock::now().time_since_epoch().count();

    // Emit ITT range pop if available
#if XSIGMA_HAS_ITT
    itt_range_pop();
#endif

    // Check if profiler is active
    if (profiler_enabled())
    {
        // In a full implementation, this would finalize the function
        // recording and add it to the event collection
    }
}

// ============================================================================
// == ScopedActivity Implementation ===========================================
// ============================================================================

ScopedActivity::ScopedActivity(const char* name) : name_(name)
{
    // Emit ITT range push if available
#if XSIGMA_HAS_ITT
    itt_range_push(name_);
#endif
}

ScopedActivity::~ScopedActivity()
{
    // Emit ITT range pop if available
#if XSIGMA_HAS_ITT
    itt_range_pop();
#endif
}

}  // namespace profiler
}  // namespace xsigma
