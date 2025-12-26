/*
 * XSigma Profiler API Implementation
 */

#include "profiler_api.h"
#if XSIGMA_HAS_KINETO
#include "kineto_shim.h"
#endif
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>

#include "itt_wrapper.h"

namespace xsigma
{
namespace profiler
{

// ============================================================================
// == ProfilerSession Implementation ==========================================
// ============================================================================

ProfilerSession& ProfilerSession::instance()
{
    static ProfilerSession instance_;
    return instance_;
}

bool ProfilerSession::start(const ProfilerConfig& config)
{
    std::scoped_lock const lock(state_mutex_);

    if (state_ != ProfilerState::Disabled)
    {
        if (config.verbose)
        {
            std::cerr << "Profiler already running" << std::endl;
        }
        return false;
    }

    config_ = config;
    state_  = ProfilerState::Ready;

    // Initialize Kineto if needed
    if ((config_.activities.count(ActivityType::CUDA) != 0u) ||
        (config_.activities.count(ActivityType::ROCM) != 0u) ||
        (config_.activities.count(ActivityType::XPU) != 0u))
    {
        initialize_kineto();
    }

    // Initialize ITT if needed
    if (config_.verbose)
    {
        initialize_itt();
    }

    // Record start time
    start_time_ns_ = std::chrono::high_resolution_clock::now().time_since_epoch().count();

    state_ = ProfilerState::Recording;

    if (config_.verbose)
    {
        std::cout << "Profiler started with " << config_.activities.size() << " activities"
                  << std::endl;
    }

    return true;
}

bool ProfilerSession::stop()
{
    std::scoped_lock const lock(state_mutex_);

    if (state_ != ProfilerState::Recording)
    {
        if (config_.verbose)
        {
            std::cerr << "Profiler not recording" << std::endl;
        }
        return false;
    }

    // Record end time
    end_time_ns_ = std::chrono::high_resolution_clock::now().time_since_epoch().count();

    // Collect events
    collect_events();

    // Finalize Kineto
    if ((config_.activities.count(ActivityType::CUDA) != 0u) ||
        (config_.activities.count(ActivityType::ROCM) != 0u) ||
        (config_.activities.count(ActivityType::XPU) != 0u))
    {
        finalize_kineto();
    }

    // Finalize ITT
    finalize_itt();

    state_ = ProfilerState::Ready;

    if (config_.verbose)
    {
        std::cout << "Profiler stopped. Collected " << events_.size() << " events" << std::endl;
    }

    return true;
}

bool ProfilerSession::is_profiling() const
{
    std::scoped_lock const lock(state_mutex_);
    return state_ == ProfilerState::Recording;
}

ProfilerState ProfilerSession::get_state() const
{
    std::scoped_lock const lock(state_mutex_);
    return state_;
}

const ProfilerConfig& ProfilerSession::get_config() const
{
    std::scoped_lock const lock(state_mutex_);
    return config_;
}

bool ProfilerSession::export_trace(const std::string& path)
{
    std::scoped_lock const lock(state_mutex_);

    if (state_ == ProfilerState::Recording)
    {
        if (config_.verbose)
        {
            std::cerr << "Cannot export while profiling" << std::endl;
        }
        return false;
    }

    try
    {
        std::ofstream file(path);
        if (!file.is_open())
        {
            if (config_.verbose)
            {
                std::cerr << "Failed to open file: " << path << std::endl;
            }
            return false;
        }

        // Write JSON header
        file << "{\n";
        file << "  \"traceEvents\": [\n";

        // Write events
        for (size_t i = 0; i < events_.size(); ++i)
        {
            file << "    " << events_[i];
            if (i < events_.size() - 1)
            {
                file << ",";
            }
            file << "\n";
        }

        file << "  ],\n";
        file << "  \"displayTimeUnit\": \"ns\",\n";
        file << R"(  "traceID": ")" << config_.trace_id << "\"\n";
        file << "}\n";

        file.close();

        if (config_.verbose)
        {
            std::cout << "Trace exported to: " << path << std::endl;
        }

        return true;
    }
    catch (const std::exception& e)
    {
        if (config_.verbose)
        {
            std::cerr << "Export failed: " << e.what() << std::endl;
        }
        return false;
    }
}

void ProfilerSession::clear()
{
    std::scoped_lock const lock(state_mutex_);
    events_.clear();
    start_time_ns_ = 0;
    end_time_ns_   = 0;
}

size_t ProfilerSession::event_count() const
{
    std::scoped_lock const lock(state_mutex_);
    return events_.size();
}

void ProfilerSession::reset()
{
    std::scoped_lock const lock(state_mutex_);
    state_ = ProfilerState::Disabled;
    events_.clear();
    start_time_ns_ = 0;
    end_time_ns_   = 0;
}

void ProfilerSession::initialize_kineto() const
{
#if XSIGMA_HAS_KINETO
    kineto_init(false, config_.verbose);
#endif
}

void ProfilerSession::initialize_itt()
{
#if XSIGMA_HAS_ITT
    itt_init();
#endif
}

void ProfilerSession::finalize_kineto()
{
#if XSIGMA_HAS_KINETO
    kineto_reset_tls();
#endif
}

void ProfilerSession::finalize_itt()
{
    // ITT cleanup is automatic
}

void ProfilerSession::collect_events() const
{
    // Simplified event collection
    // In a full implementation, this would:
    // 1. Collect CPU events from RecordFunction callbacks
    // 2. Collect GPU events from Kineto
    // 3. Merge and correlate events
    // 4. Build event tree
    // 5. Export to JSON

    if (config_.verbose)
    {
        std::cout << "Collecting events from " << start_time_ns_ << " to " << end_time_ns_
                  << std::endl;
    }
}

// ============================================================================
// == Global API Functions ====================================================
// ============================================================================

bool profiler_enabled()
{
    return ProfilerSession::instance().get_state() != ProfilerState::Disabled;
}

ProfilerState get_profiler_state()
{
    return ProfilerSession::instance().get_state();
}

const ProfilerConfig& get_profiler_config()
{
    return ProfilerSession::instance().get_config();
}

}  // namespace profiler
}  // namespace xsigma
