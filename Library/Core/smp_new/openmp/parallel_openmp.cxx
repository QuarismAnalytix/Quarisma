#include "smp_new/openmp/parallel_openmp.h"

#include <atomic>
#include <sstream>
#include <stdexcept>

#ifdef _OPENMP
#include <omp.h>
#endif

#if XSIGMA_HAS_MKL
#include <mkl.h>
#endif

namespace xsigma::smp_new::openmp
{

namespace
{
// OpenMP backend state
std::atomic<bool> g_openmp_initialized{false};
std::atomic<int>  g_num_threads{-1};
thread_local int  g_thread_id{0};

}  // namespace

void InitializeOpenMPBackend()
{
#ifdef _OPENMP
    if (!g_openmp_initialized.exchange(true))
    {
// Initialize OpenMP
#pragma omp parallel
        {
#pragma omp single
            {
                // Get default number of threads
                int nthreads = omp_get_max_threads();
                g_num_threads.store(nthreads);
            }
        }
    }
#else
    throw std::runtime_error("OpenMP is not available. Compile with OpenMP support.");
#endif
}

void ShutdownOpenMPBackend()
{
    g_openmp_initialized.store(false);
    g_num_threads.store(-1);
}

bool IsOpenMPBackendInitialized()
{
    return g_openmp_initialized.load();
}

bool IsOpenMPAvailable()
{
#ifdef _OPENMP
    return true;
#else
    return false;
#endif
}

void SetNumOpenMPThreads(int nthreads)
{
#ifdef _OPENMP
    if (nthreads <= 0)
    {
        throw std::runtime_error("Expected positive number of threads");
    }

    g_num_threads.store(nthreads);

#pragma omp parallel
    {
#pragma omp single
        {
            omp_set_num_threads(nthreads);
        }
    }

#if XSIGMA_HAS_MKL
    mkl_set_num_threads_local(nthreads);
    mkl_set_dynamic(false);
#endif
#else
    (void)nthreads;  // Suppress unused parameter warning
    throw std::runtime_error("OpenMP is not available");
#endif
}

int GetNumOpenMPThreads()
{
#ifdef _OPENMP
    int nthreads = g_num_threads.load();
    if (nthreads > 0)
    {
        return nthreads;
    }

#pragma omp parallel
    {
#pragma omp single
        {
            nthreads = omp_get_max_threads();
            g_num_threads.store(nthreads);
        }
    }

    return nthreads;
#else
    throw std::runtime_error("OpenMP is not available");
#endif
}

int GetOpenMPThreadNum()
{
#ifdef _OPENMP
    return omp_get_thread_num();
#else
    return 0;
#endif
}

bool InOpenMPParallelRegion()
{
#ifdef _OPENMP
    return omp_in_parallel();
#else
    return false;
#endif
}

std::string GetOpenMPBackendInfo()
{
    std::ostringstream ss;

    ss << "OpenMP Backend Information:\n";
    ss << "  Available: " << (IsOpenMPAvailable() ? "Yes" : "No") << "\n";
    ss << "  Initialized: " << (IsOpenMPBackendInitialized() ? "Yes" : "No") << "\n";

#ifdef _OPENMP
    ss << "  OpenMP Version: " << _OPENMP << "\n";
    try
    {
        ss << "  Num Threads: " << GetNumOpenMPThreads() << "\n";
    }
    catch (const std::exception&)
    {
        ss << "  Num Threads: <error>\n";
    }
#else
    ss << "  OpenMP Version: Not available\n";
#endif

#if XSIGMA_HAS_MKL
    ss << "  MKL Integration: Yes\n";
#else
    ss << "  MKL Integration: No\n";
#endif

    return ss.str();
}

}  // namespace xsigma::smp_new::openmp
