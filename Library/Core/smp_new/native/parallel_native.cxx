#include "smp_new/native/parallel_native.h"

#include <atomic>
#include <sstream>

#include "smp_new/openmp/parallel_openmp.h"
#include "smp_new/tbb/parallel_tbb.h"

namespace xsigma::smp_new::native
{

namespace
{

std::atomic<bool>        g_backend_initialized{false};
std::atomic<bool>        g_native_backend_initialized{false};
std::atomic<BackendType> g_current_backend{BackendType::NATIVE};

}  // namespace

void InitializeBackend(BackendType backend)
{
    if (backend == BackendType::AUTO)
    {
        // Auto-select: prefer TBB > OpenMP > Native
        if (tbb::IsTBBAvailable())
        {
            backend = BackendType::TBB;
        }
        else if (openmp::IsOpenMPAvailable())
        {
            backend = BackendType::OPENMP;
        }
        else
        {
            backend = BackendType::NATIVE;
        }
    }

    g_current_backend.store(backend);

    switch (backend)
    {
    case BackendType::NATIVE:
        InitializeNativeBackend();
        break;
    case BackendType::OPENMP:
        openmp::InitializeOpenMPBackend();
        break;
    case BackendType::TBB:
        tbb::InitializeTBBBackend();
        break;
    case BackendType::AUTO:
        // Should not reach here
        break;
    }

    g_backend_initialized.store(true);
}

void InitializeNativeBackend()
{
    bool expected = false;
    g_native_backend_initialized.compare_exchange_strong(expected, true);
}

void ShutdownBackend()
{
    BackendType const backend = g_current_backend.load();

    switch (backend)
    {
    case BackendType::NATIVE:
        ShutdownNativeBackend();
        break;
    case BackendType::OPENMP:
        openmp::ShutdownOpenMPBackend();
        break;
    case BackendType::TBB:
        tbb::ShutdownTBBBackend();
        break;
    case BackendType::AUTO:
        break;
    }

    g_backend_initialized.store(false);
}

void ShutdownNativeBackend()
{
    g_native_backend_initialized = false;
}

bool IsBackendInitialized()
{
    return g_backend_initialized.load();
}

bool IsNativeBackendInitialized()
{
    return g_native_backend_initialized.load();
}

BackendType GetCurrentBackend()
{
    return g_current_backend.load();
}

std::string GetBackendInfo()
{
    std::ostringstream oss;
    oss << "XSigma SMP_NEW Backend Information:\n";
    oss << "  Current Backend: ";

    BackendType const backend = g_current_backend.load();
    switch (backend)
    {
    case BackendType::NATIVE:
        oss << "Native (std::thread)\n";
        break;
    case BackendType::OPENMP:
        oss << "OpenMP\n";
        break;
    case BackendType::TBB:
        oss << "Intel TBB\n";
        break;
    case BackendType::AUTO:
        oss << "Auto\n";
        break;
    }

    oss << "  Initialized: " << (IsBackendInitialized() ? "Yes" : "No") << "\n";
    oss << "  OpenMP Available: " << (openmp::IsOpenMPAvailable() ? "Yes" : "No") << "\n";
    oss << "  TBB Available: " << (tbb::IsTBBAvailable() ? "Yes" : "No") << "\n";
    oss << "\n";

    if (backend == BackendType::NATIVE)
    {
        oss << GetNativeBackendInfo();
    }
    else if (backend == BackendType::OPENMP)
    {
        oss << openmp::GetOpenMPBackendInfo();
    }
    else if (backend == BackendType::TBB)
    {
        oss << tbb::GetTBBBackendInfo();
    }

    return oss.str();
}

std::string GetNativeBackendInfo()
{
    std::ostringstream oss;
    oss << "Native Backend (std::thread):\n";
    oss << "  Version: 1.0\n";
    oss << "  Type: std::thread-based\n";
    oss << "  Features:\n";
    oss << "    - Master-worker pattern\n";
    oss << "    - Lazy initialization\n";
    oss << "    - Separate intra-op and inter-op pools\n";
    oss << "    - Exception handling\n";
    oss << "    - NUMA support\n";
    oss << "  Status: " << (IsNativeBackendInitialized() ? "Initialized" : "Not initialized")
        << "\n";
    return oss.str();
}

}  // namespace xsigma::smp_new::native
