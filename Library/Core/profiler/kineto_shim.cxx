/*
 * XSigma Kineto Shim Implementation
 *
 * Direct libkineto integration matching PyTorch's implementation.
 *
 * Note: This file is only compiled when XSIGMA_HAS_KINETO=ON.
 * Build-time exclusion is handled in CMakeLists.txt.
 */

#include "kineto_shim.h"

// Suppress MSVC warnings for Kineto headers
// Kineto headers have several issues that trigger MSVC warnings:
// - C4100: unreferenced formal parameter
// - C4245: conversion from 'int' to 'uint8_t', signed/unsigned mismatch
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100)  // unreferenced formal parameter
#pragma warning(disable : 4245)  // signed/unsigned mismatch
#endif

#include <libkineto.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <mutex>

namespace xsigma
{
namespace profiler
{

namespace
{
// Thread-local state for Kineto
thread_local bool g_kineto_initialized = false;
std::mutex        g_kineto_init_mutex;
}  // namespace

void kineto_init(bool cpu_only, bool log_on_error)
{
    std::scoped_lock const lock(g_kineto_init_mutex);

    if (g_kineto_initialized)
    {
        return;
    }

    // Initialize libkineto
    libkineto_init(cpu_only, log_on_error);

    // Suppress log messages if requested
    if (log_on_error)
    {
        libkineto::api().suppressLogMessages();
    }

    g_kineto_initialized = true;
}

bool kineto_is_profiler_registered()
{
    return libkineto::api().isProfilerRegistered();
}

bool kineto_is_profiler_initialized()
{
    return libkineto::api().isProfilerInitialized();
}

void kineto_prepare_trace(
    const std::set<libkineto::ActivityType>& activities, const std::string& config_str)
{
    // Reset thread-local state
    libkineto::api().resetKinetoTLS();

    // Initialize if not already done
    if (!kineto_is_profiler_registered())
    {
        kineto_init(false, true);
    }

    // Initialize profiler if needed
    if (!kineto_is_profiler_initialized())
    {
        libkineto::api().initProfilerIfRegistered();
    }

    // Prepare trace with specified activities
    libkineto::api().activityProfiler().prepareTrace(activities, config_str);
}

void kineto_start_trace()
{
    libkineto::api().activityProfiler().startTrace();
}

void* kineto_stop_trace()
{
    auto trace = libkineto::api().activityProfiler().stopTrace();
    // Return raw pointer - caller is responsible for cleanup
    return trace.release();
}

void kineto_reset_tls()
{
    libkineto::api().resetKinetoTLS();
}

}  // namespace profiler
}  // namespace xsigma
