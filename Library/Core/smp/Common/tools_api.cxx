// This file incorporates code from the Visualization Toolkit (VTK) and remains subject to the BSD-3-Clause VTK license.

#include "smp/Common/tools_api.h"

#include <algorithm>  // For std::toupper
#include <cctype>     // For std::toupper
#include <cstdlib>    // For std::getenv
#include <memory>
#include <string>  // For std::string

#include "smp/Common/tools_impl.h"

namespace xsigma::detail::smp
{

//------------------------------------------------------------------------------
tools_api::tools_api()
    :
#if XSIGMA_HAS_TBB
      TBBBackend(std::make_unique<tools_impl<BackendType::TBB>>())
#else
      STDThreadBackend(std::make_unique<tools_impl<BackendType::STDThread>>())
#endif
{
    const char* xsigmaSMPBackendInUse = std::getenv("XSIGMA_SMP_BACKEND_IN_USE");
    if (xsigmaSMPBackendInUse != nullptr)
    {
        this->SetBackend(xsigmaSMPBackendInUse);
    }

    this->RefreshNumberOfThread();
}

//------------------------------------------------------------------------------
// Must NOT be initialized. Default initialization to zero is necessary.
static tools_api* toolsAPIInstanceAsPointer = nullptr;

//------------------------------------------------------------------------------
tools_api& tools_api::GetInstance()
{
    return *toolsAPIInstanceAsPointer;
}

//------------------------------------------------------------------------------
void tools_api::ClassInitialize()
{
    if (toolsAPIInstanceAsPointer == nullptr)
    {
        toolsAPIInstanceAsPointer = new tools_api;
    }
}

//------------------------------------------------------------------------------
void tools_api::ClassFinalize()
{
    delete toolsAPIInstanceAsPointer;
    toolsAPIInstanceAsPointer = nullptr;
}

//------------------------------------------------------------------------------
BackendType tools_api::GetBackendType()
{
    return this->ActivatedBackend;
}

//------------------------------------------------------------------------------
const char* tools_api::GetBackend()
{
#if XSIGMA_HAS_TBB
    return "TBB";
#else
    return "STDThread";
#endif
}

//------------------------------------------------------------------------------
bool tools_api::SetBackend(const char* type)
{
    std::string backend((type != nullptr) ? type : "");
    std::transform(backend.cbegin(), backend.cend(), backend.begin(), ::toupper);

#if XSIGMA_HAS_TBB
    this->ActivatedBackend = BackendType::TBB;
#else
    this->ActivatedBackend = BackendType::STDThread;
#endif

    this->RefreshNumberOfThread();
    return true;
}

//------------------------------------------------------------------------------
void tools_api::Initialize(int numThreads)
{
    this->DesiredNumberOfThread = numThreads;
    this->RefreshNumberOfThread();
}

//------------------------------------------------------------------------------
void tools_api::RefreshNumberOfThread()
{
    const int numThreads = this->DesiredNumberOfThread;

#if XSIGMA_HAS_TBB
    this->TBBBackend->Initialize(numThreads);
#else
    this->STDThreadBackend->Initialize(numThreads);
#endif
}

//------------------------------------------------------------------------------
int tools_api::GetEstimatedDefaultNumberOfThreads()
{
#if XSIGMA_HAS_TBB
    return this->TBBBackend->GetEstimatedDefaultNumberOfThreads();
#else
    return this->STDThreadBackend->GetEstimatedDefaultNumberOfThreads();
#endif
}

//------------------------------------------------------------------------------
int tools_api::GetEstimatedNumberOfThreads()
{
#if XSIGMA_HAS_TBB
    return this->TBBBackend->GetEstimatedNumberOfThreads();
#else
    return this->STDThreadBackend->GetEstimatedNumberOfThreads();
#endif
}

//------------------------------------------------------------------------------
void tools_api::SetNestedParallelism(bool isNested)
{
#if XSIGMA_HAS_TBB
    return this->TBBBackend->SetNestedParallelism(isNested);
#else
    return this->STDThreadBackend->SetNestedParallelism(isNested);
#endif
}

//------------------------------------------------------------------------------
bool tools_api::GetNestedParallelism()
{
#if XSIGMA_HAS_TBB
    return this->TBBBackend->GetNestedParallelism();
#else
    return this->STDThreadBackend->GetNestedParallelism();
#endif
}

//------------------------------------------------------------------------------
bool tools_api::IsParallelScope()
{
#if XSIGMA_HAS_TBB
    return this->TBBBackend->IsParallelScope();
#else
    return this->STDThreadBackend->IsParallelScope();
#endif
}

//------------------------------------------------------------------------------
bool tools_api::GetSingleThread()
{
#if XSIGMA_HAS_TBB
    return this->TBBBackend->GetSingleThread();
#else
    return this->STDThreadBackend->GetSingleThread();
#endif
}

//------------------------------------------------------------------------------
// Must NOT be initialized. Default initialization to zero is necessary.
static unsigned int toolsAPIInitializeCount = 0;

//------------------------------------------------------------------------------
toolsAPIInitialize::toolsAPIInitialize()
{
    if (++toolsAPIInitializeCount == 1)
    {
        tools_api::ClassInitialize();
    }
}

//------------------------------------------------------------------------------
toolsAPIInitialize::~toolsAPIInitialize()
{
    if (--toolsAPIInitializeCount == 0)
    {
        tools_api::ClassFinalize();
    }
}

}  // namespace xsigma::detail::smp
