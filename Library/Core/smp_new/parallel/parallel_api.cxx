#include "smp_new/parallel/parallel_api.h"

#include <atomic>
#include <memory>
#include <sstream>
#include <thread>

#include "smp_new/core/thread_pool.h"
#include "smp_new/native/parallel_native.h"
#include "smp_new/openmp/parallel_openmp.h"
#include "smp_new/parallel/parallelize_1d.h"
#include "smp_new/tbb/parallel_tbb.h"

namespace xsigma::smp_new::parallel
{

namespace
{

// Atomic state machine for lazy initialization
constexpr int kNotSet   = -1;
constexpr int kConsumed = -2;

// Global thread pool instances
std::atomic<int> g_num_intraop_threads{kNotSet};
std::atomic<int> g_num_interop_threads{kNotSet};

std::shared_ptr<core::TaskThreadPoolBase> g_intraop_pool;
std::shared_ptr<core::TaskThreadPoolBase> g_interop_pool;

std::mutex g_pool_mutex;

// Thread-local state for intra-op parallelism
thread_local bool g_in_intraop_region = false;

// Thread-local state for parallel region tracking
thread_local int  g_thread_num         = 0;
thread_local bool g_in_parallel_region = false;

// Backend state
std::atomic<int> g_backend_initialized{0};

}  // namespace

void launch(std::function<void()> fn)
{
    internal::GetInteropPool().Run(std::move(fn));
}

void intraop_launch(std::function<void()> fn)
{
    // Check intra-op pool availability (not inter-op pool)
    if (!g_in_intraop_region && internal::GetIntraopPool().NumAvailable() > 0)
    {
        g_in_intraop_region = true;
        try
        {
            internal::GetIntraopPool().Run(std::move(fn));
        }
        catch (...)
        {
            g_in_intraop_region = false;
            throw;
        }
        g_in_intraop_region = false;
    }
    else
    {
        // Execute inline if nested or pool exhausted
        fn();
    }
}

void set_num_intraop_threads(int nthreads)
{
    // Validate input: thread count must be positive
    if (nthreads <= 0)
    {
        return;
    }

    int expected = kNotSet;
    if (!g_num_intraop_threads.compare_exchange_strong(expected, nthreads))
    {
        if (expected != kConsumed)
        {
            // Already set to a different value
            return;
        }
    }
}

size_t get_num_intraop_threads()
{
    int const num_threads = g_num_intraop_threads.load();
    if (num_threads == kNotSet || num_threads == kConsumed)
    {
        return core::TaskThreadPoolBase::DefaultNumThreads();
    }
    return static_cast<size_t>(num_threads);
}

void set_num_interop_threads(int nthreads)
{
    // Validate input: thread count must be positive
    if (nthreads <= 0)
    {
        return;
    }

    int expected = kNotSet;
    if (!g_num_interop_threads.compare_exchange_strong(expected, nthreads))
    {
        if (expected != kConsumed)
        {
            // Already set to a different value
            return;
        }
    }
}

size_t get_num_interop_threads()
{
    int const num_threads = g_num_interop_threads.load();
    if (num_threads == kNotSet || num_threads == kConsumed)
    {
        return core::TaskThreadPoolBase::DefaultNumThreads();
    }
    return static_cast<size_t>(num_threads);
}

void set_backend(int backend)
{
    if (g_backend_initialized.exchange(1) == 0)
    {
        native::InitializeBackend(static_cast<native::BackendType>(backend));
    }
}

int get_backend()
{
    return static_cast<int>(native::GetCurrentBackend());
}

bool is_openmp_available()
{
    return openmp::IsOpenMPAvailable();
}

bool is_tbb_available()
{
    return tbb::IsTBBAvailable();
}

int get_thread_num()
{
    return g_thread_num;
}

bool in_parallel_region()
{
    return g_in_parallel_region;
}

std::string get_parallel_info()
{
    std::stringstream ss;
    ss << "Backend: " << native::GetBackendInfo() << "\n";
    ss << "Intra-op threads: " << get_num_intraop_threads() << "\n";
    ss << "Inter-op threads: " << get_num_interop_threads() << "\n";
    // cppcheck-suppress knownConditionTrueFalse
    ss << "In parallel region: " << (in_parallel_region() ? "yes" : "no") << "\n";
    ss << "Current thread ID: " << get_thread_num();
    return ss.str();
}

}  // namespace xsigma::smp_new::parallel

// Namespace for internal thread state management
namespace xsigma::smp_new::parallel::internal
{

void set_thread_num(int thread_id)
{
    g_thread_num = thread_id;
}

void set_in_parallel_region(bool in_region)
{
    g_in_parallel_region = in_region;
}

core::TaskThreadPoolBase& GetIntraopPool()
{
    if (!g_intraop_pool)
    {
        std::scoped_lock const lock(g_pool_mutex);
        // Double-checked locking pattern - recheck after acquiring lock
        // cppcheck-suppress identicalInnerCondition
        if (!g_intraop_pool)  // NOLINT(clang-analyzer-core.NullDereference)
        {
            int num_threads = g_num_intraop_threads.exchange(kConsumed);
            if (num_threads == kNotSet)
            {
                num_threads = static_cast<int>(core::TaskThreadPoolBase::DefaultNumThreads());
            }
            g_intraop_pool = core::CreateThreadPool(num_threads);
        }
    }
    return *g_intraop_pool;
}

core::TaskThreadPoolBase& GetInteropPool()
{
    if (!g_interop_pool)
    {
        std::scoped_lock const lock(g_pool_mutex);
        // Double-checked locking pattern - recheck after acquiring lock
        // cppcheck-suppress identicalInnerCondition
        if (!g_interop_pool)  // NOLINT(clang-analyzer-core.NullDereference)
        {
            int num_threads = g_num_interop_threads.exchange(kConsumed);
            if (num_threads == kNotSet)
            {
                num_threads = static_cast<int>(core::TaskThreadPoolBase::DefaultNumThreads());
            }
            g_interop_pool = core::CreateThreadPool(num_threads);
        }
    }
    return *g_interop_pool;
}

}  // namespace xsigma::smp_new::parallel::internal
