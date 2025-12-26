#include "smp_new/tbb/parallel_tbb.h"

#include <atomic>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

// Check if XSIGMA_HAS_TBB is defined, default to 0 if not
#ifndef XSIGMA_HAS_TBB
#define XSIGMA_HAS_TBB 0
#endif

#if XSIGMA_HAS_TBB
#include <tbb/blocked_range.h>
#include <tbb/global_control.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <tbb/task_arena.h>
#include <tbb/task_group.h>
#include <tbb/version.h>
#endif

namespace xsigma::smp_new::tbb
{

// TBB backend state
XSIGMA_API std::atomic<bool> g_tbb_initialized{false};  //NOLINT
static std::atomic<int>      g_num_threads{-1};

#if XSIGMA_HAS_TBB
// TBB global control for thread count
namespace
{
std::unique_ptr<::tbb::global_control> g_global_control;
std::mutex                             g_tbb_mutex;
}  // namespace

// Thread-local state (exported for template access)
thread_local int  g_thread_id{0};
thread_local bool g_in_parallel_region{false};
#endif

void InitializeTBBBackend()
{
#if XSIGMA_HAS_TBB
    if (!g_tbb_initialized.exchange(true))
    {
        std::scoped_lock const lock(g_tbb_mutex);

        // Initialize TBB with default number of threads
        int nthreads = ::tbb::task_arena::automatic;

        // Check environment variable for thread count
        const char* env_threads = std::getenv("TBB_NUM_THREADS");
        if (env_threads != nullptr)
        {
            // Parse environment variable manually to avoid exceptions
            char*      endptr = nullptr;  //NOLINT
            const long value  = std::strtol(env_threads, &endptr, 10);

            // Check if conversion was successful and value is valid
            if (endptr != env_threads && *endptr == '\0' && value > 0 &&
                value <= std::numeric_limits<int>::max())
            {
                nthreads = static_cast<int>(value);
            }
            // Invalid TBB_NUM_THREADS value, ignore and use default
        }

        // Create global control for thread count
        if (nthreads != ::tbb::task_arena::automatic)
        {
            g_global_control = std::make_unique<::tbb::global_control>(
                ::tbb::global_control::max_allowed_parallelism, nthreads);
            g_num_threads.store(nthreads);
        }
        else
        {
            // Get default thread count from TBB
            ::tbb::task_arena const arena;
            g_num_threads.store(arena.max_concurrency());
        }
    }
#else
    // TBB is not available - do nothing
    (void)0;  // Suppress unused variable warnings
#endif
}

void ShutdownTBBBackend()
{
#if XSIGMA_HAS_TBB
    if (g_tbb_initialized.exchange(false))
    {
        std::scoped_lock const lock(g_tbb_mutex);

        // Release global control
        g_global_control.reset();
        g_num_threads.store(-1);
    }
#endif
}

bool IsTBBBackendInitialized()
{
    return g_tbb_initialized.load();
}

bool IsTBBAvailable()
{
#if XSIGMA_HAS_TBB
    return true;
#else
    return false;
#endif
}

void SetNumTBBThreads(int nthreads)
{
#if XSIGMA_HAS_TBB
    if (nthreads <= 0)
    {
        return;
    }

    std::scoped_lock const lock(g_tbb_mutex);

    // Update global control
    g_global_control = std::make_unique<::tbb::global_control>(
        ::tbb::global_control::max_allowed_parallelism, nthreads);

    g_num_threads.store(nthreads);
#else
    (void)nthreads;  // Suppress unused parameter warning
#endif
}

int GetNumTBBThreads()
{
    return g_num_threads.load();
}

int GetTBBThreadNum()
{
#if XSIGMA_HAS_TBB
    return g_thread_id;
#else
    return 0;
#endif
}

bool InTBBParallelRegion()
{
#if XSIGMA_HAS_TBB
    return g_in_parallel_region;
#else
    return false;
#endif
}

void ParallelForTBB(
    int64_t                                      begin,
    int64_t                                      end,
    int64_t                                      grain_size,
    const std::function<void(int64_t, int64_t)>& func)
{
#if XSIGMA_HAS_TBB
    if (!g_tbb_initialized.load())
    {
        InitializeTBBBackend();
    }

    int64_t n = end - begin;

    // Determine grain size if auto (grain_size <= 0)
    if (grain_size <= 0)
    {
        auto num_threads = static_cast<int64_t>(GetNumTBBThreads());
        if (num_threads <= 0)
        {
            num_threads = 1;
        }
        grain_size = std::max(static_cast<int64_t>(1), n / (num_threads * 4));
    }

    // Set parallel region flag
    bool const was_in_parallel = GetInParallelRegion();
    SetInParallelRegion(true);

    // Execute parallel for
    // Note: TBB may throw exceptions internally, but we let them propagate
    // as this is part of TBB's contract. The calling code should handle
    // exceptions if needed.
    ::tbb::parallel_for(
        ::tbb::blocked_range<int64_t>(begin, end, grain_size),
        [&func](const ::tbb::blocked_range<int64_t>& range) { func(range.begin(), range.end()); });

    // Restore parallel region flag
    SetInParallelRegion(was_in_parallel);
#else
    // TBB is not available - fallback to serial execution
    (void)grain_size;  // Suppress unused parameter warning
    func(begin, end);
#endif
}

std::string GetTBBBackendInfo()
{
    std::ostringstream oss;
    oss << "TBB Backend:\n";

#if XSIGMA_HAS_TBB
    oss << "  Version: " << TBB_VERSION_MAJOR << "." << TBB_VERSION_MINOR << "\n";
    oss << "  Type: Intel Threading Building Blocks\n";
    oss << "  Features:\n";
    oss << "    - Task-based parallelism\n";
    oss << "    - Work-stealing scheduler\n";
    oss << "    - Automatic load balancing\n";
    oss << "    - Nested parallelism support\n";
    oss << "    - Cache-aware scheduling\n";
    oss << "    - Exception propagation\n";
    oss << "  Status: " << (IsTBBBackendInitialized() ? "Initialized" : "Not initialized") << "\n";
    oss << "  Threads: " << GetNumTBBThreads() << "\n";
#else
    oss << "  Status: Not available (compile with XSIGMA_HAS_TBB=1)\n";
#endif

    return oss.str();
}

#if XSIGMA_HAS_TBB
bool GetInParallelRegion()
{
    return g_in_parallel_region;
}

void SetInParallelRegion(bool in_region)
{
    g_in_parallel_region = in_region;
}
#else
// Stub implementations when TBB is not available
bool GetInParallelRegion()
{
    return false;
}

void SetInParallelRegion(bool in_region)
{
    // No-op when TBB is not available
    (void)in_region;
}
#endif

}  // namespace xsigma::smp_new::tbb
