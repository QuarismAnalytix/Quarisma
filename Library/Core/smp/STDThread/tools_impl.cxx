

// This file incorporates code from the Visualization Toolkit (VTK) and remains subject to the BSD-3-Clause VTK license.

#include "smp/STDThread/tools_impl.hxx"

#include <algorithm>
#include <cstdlib>  // for atoi, std::getenv
#include <thread>   // for thread::id, thread, get_id

#include "smp/Common/tools_impl.h"

namespace xsigma::detail::smp
{
static int specifiedNumThreadsSTD;  // Default initialized to zero

//------------------------------------------------------------------------------
int GetNumberOfThreadsSTDThread()
{
    return specifiedNumThreadsSTD != 0 ? specifiedNumThreadsSTD
                                       : (int)std::thread::hardware_concurrency();
}

//------------------------------------------------------------------------------
template <>
void tools_impl<BackendType::STDThread>::Initialize(int numThreads)
{
    const int maxThreads = (int)std::thread::hardware_concurrency();
    if (numThreads == 0)
    {
        const char* xsigmaSmpNumThreads = std::getenv("XSIGMA_SMP_MAX_THREADS");
        if (xsigmaSmpNumThreads != nullptr)
        {
            numThreads = std::atoi(xsigmaSmpNumThreads);
        }
        else
        {
            specifiedNumThreadsSTD = 0;
        }
    }
    if (numThreads > 0)
    {
        numThreads             = std::min(numThreads, maxThreads);
        specifiedNumThreadsSTD = numThreads;
    }
}

//------------------------------------------------------------------------------
template <>
int tools_impl<BackendType::STDThread>::GetEstimatedNumberOfThreads()
{
    return specifiedNumThreadsSTD > 0 ? specifiedNumThreadsSTD
                                      : (int)std::thread::hardware_concurrency();
}

//------------------------------------------------------------------------------
template <>
int tools_impl<BackendType::STDThread>::GetEstimatedDefaultNumberOfThreads()
{
    return (int)std::thread::hardware_concurrency();
}

//------------------------------------------------------------------------------
template <>
bool tools_impl<BackendType::STDThread>::GetSingleThread()
{
    return thread_pool::GetInstance().GetSingleThread();
}

//------------------------------------------------------------------------------
template <>
bool tools_impl<BackendType::STDThread>::IsParallelScope()
{
    return thread_pool::GetInstance().IsParallelScope();
}

// Explicit template instantiation for shared library builds
template class tools_impl<BackendType::STDThread>;

}  // namespace xsigma::detail::smp
