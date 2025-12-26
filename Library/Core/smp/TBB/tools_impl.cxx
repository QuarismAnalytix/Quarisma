

// This file incorporates code from the Visualization Toolkit (VTK) and remains subject to the BSD-3-Clause VTK license.

#include "smp/Common/tools_impl.h"

#include <cstdlib>  // For std::getenv()
#include <mutex>    // For std::mutex
#include <stack>    // For std::stack

#include "smp/TBB/tools_impl.hxx"

#ifdef _MSC_VER
#pragma push_macro("__TBB_NO_IMPLICIT_LINKAGE")
#define __TBB_NO_IMPLICIT_LINKAGE 1  //NOLINT
#endif

#include <tbb/task_arena.h>  // For tbb:task_arena

#ifdef _MSC_VER
#pragma pop_macro("__TBB_NO_IMPLICIT_LINKAGE")
#endif

namespace xsigma::detail::smp
{

static tbb::task_arena* taskArena;
static std::mutex*      toolsCS;
static std::stack<int>* threadIdStack;
static std::mutex*      threadIdStackLock;
static int              specifiedNumThreadsTBB;  // Default initialized to zero

//------------------------------------------------------------------------------
// Must NOT be initialized. Default initialization to zero is necessary.
static unsigned int tools_implTBBInitializeCount;

//------------------------------------------------------------------------------
tools_implTBBInitialize::tools_implTBBInitialize()
{
    if (++tools_implTBBInitializeCount == 1)
    {
        taskArena         = new tbb::task_arena;
        toolsCS           = new std::mutex;
        threadIdStack     = new std::stack<int>;
        threadIdStackLock = new std::mutex;
    }
}

//------------------------------------------------------------------------------
tools_implTBBInitialize::~tools_implTBBInitialize()
{
    if (--tools_implTBBInitializeCount == 0)
    {
        delete taskArena;
        taskArena = nullptr;

        delete toolsCS;
        toolsCS = nullptr;

        delete threadIdStack;
        threadIdStack = nullptr;

        delete threadIdStackLock;
        threadIdStackLock = nullptr;
    }
}

//------------------------------------------------------------------------------
template <>
void tools_impl<BackendType::TBB>::Initialize(int numThreads)
{
    toolsCS->lock();

    if (numThreads == 0)
    {
        const char* xsigmaSmpNumThreads = std::getenv("XSIGMA_SMP_MAX_THREADS");
        if (xsigmaSmpNumThreads != nullptr)
        {
            numThreads = std::atoi(xsigmaSmpNumThreads);
        }
        else if (taskArena->is_active())
        {
            taskArena->terminate();
            specifiedNumThreadsTBB = 0;
        }
    }
    if (numThreads > 0 && numThreads <= taskArena->max_concurrency())
    {
        if (taskArena->is_active())
        {
            taskArena->terminate();
        }
        taskArena->initialize(numThreads);
        specifiedNumThreadsTBB = numThreads;
    }

    toolsCS->unlock();
}

//------------------------------------------------------------------------------
template <>
int tools_impl<BackendType::TBB>::GetEstimatedNumberOfThreads()
{
    return specifiedNumThreadsTBB > 0 ? specifiedNumThreadsTBB : taskArena->max_concurrency();
}

//------------------------------------------------------------------------------
template <>
int tools_impl<BackendType::TBB>::GetEstimatedDefaultNumberOfThreads()
{
    return taskArena->max_concurrency();
}

//------------------------------------------------------------------------------
template <>
bool tools_impl<BackendType::TBB>::GetSingleThread()
{
    std::scoped_lock const lock(*threadIdStackLock);
    if (threadIdStack->empty())
    {
        return false;
    }
    return threadIdStack->top() == tbb::this_task_arena::current_thread_index();
}

//------------------------------------------------------------------------------
void tools_implForTBB(
    int first, int last, int grain, ExecuteFunctorPtrType functorExecuter, void* functor)
{
    threadIdStackLock->lock();
    threadIdStack->emplace(tbb::this_task_arena::current_thread_index());
    threadIdStackLock->unlock();

    if (taskArena->is_active())
    {
        taskArena->execute([&] { functorExecuter(functor, first, last, grain); });
    }
    else
    {
        functorExecuter(functor, first, last, grain);
    }

    threadIdStackLock->lock();
    threadIdStack->pop();
    threadIdStackLock->unlock();
}

}  // namespace xsigma::detail::smp
