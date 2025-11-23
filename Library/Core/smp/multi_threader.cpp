
// This file incorporates code from the Visualization Toolkit (VTK) and remains subject to the BSD-3-Clause VTK license.

#include "multi_threader.h"

#include <algorithm>  // for clamp, min

#include "logging/logger.h"
#include "util/exception.h"  // for XSIGMA_CHECK
#include "xsigma_threads.h"

// Need to define "xsigmaExternCThreadFunctionType" to avoid warning on some
// platforms about passing function pointer to an argument expecting an
// extern "C" function.  Placing the typedef of the function pointer type
// inside an extern "C" block solves this problem.
#ifdef XSIGMA_HAS_PTHREADS
#include <pthread.h>
#include <unistd.h>
extern "C"
{
    using xsigmaExternCThreadFunctionType = void* (*)(void*);
}
#else
using xsigmaExternCThreadFunctionType = xsigmaThreadFunctionType;
#endif

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <sys/types.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef min
#undef min
#endif

namespace xsigma
{
// Initialize static member that controls global maximum number of threads
static int xsigmaMultiThreaderGlobalMaximumNumberOfThreads = 0;

void multi_threader::SetGlobalMaximumNumberOfThreads(int val)
{
    if (val == xsigmaMultiThreaderGlobalMaximumNumberOfThreads)
    {
        return;
    }
    xsigmaMultiThreaderGlobalMaximumNumberOfThreads = val;
}

int multi_threader::GetGlobalMaximumNumberOfThreads()
{
    return xsigmaMultiThreaderGlobalMaximumNumberOfThreads;
}

int multi_threader::GetGlobalStaticMaximumNumberOfThreads()
{
    return XSIGMA_MAX_THREADS;
}

// 0 => Not initialized.
static int xsigmaMultiThreaderGlobalDefaultNumberOfThreads = 0;

void multi_threader::SetGlobalDefaultNumberOfThreads(int val)
{
    if (val == xsigmaMultiThreaderGlobalDefaultNumberOfThreads)
    {
        return;
    }
    xsigmaMultiThreaderGlobalDefaultNumberOfThreads = val;
}

int multi_threader::GetGlobalDefaultNumberOfThreads()
{
    if (xsigmaMultiThreaderGlobalDefaultNumberOfThreads == 0)
    {
        int num = 1;  // default is 1

#ifdef XSIGMA_HAS_PTHREADS
        // Default the number of threads to be the number of available
        // processors if we are using pthreads()
#ifdef _SC_NPROCESSORS_ONLN
        num = sysconf(_SC_NPROCESSORS_ONLN);  // NOLINT
#elif defined(_SC_NPROC_ONLN)
        num = sysconf(_SC_NPROC_ONLN);
#endif
#endif

#ifdef __APPLE__
        // Determine the number of CPU cores.
        // hw.logicalcpu takes into account cores/CPUs that are
        // disabled because of power management.
        size_t    dataLen = sizeof(int);  // 'num' is an 'int'
        int const result  = sysctlbyname("hw.logicalcpu", &num, &dataLen, nullptr, 0);
        if (result == -1)
        {
            num = 1;
        }
#endif

#ifdef _WIN32
        {
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
            num = (int)sysInfo.dwNumberOfProcessors;
        }
#endif

#ifndef XSIGMA_HAS_WIN32_THREADS
#ifndef XSIGMA_HAS_PTHREADS
        // If we are not multithreading, the number of threads should
        // always be 1
        // cppcheck-suppress redundantAssignment
        // Explanation: This assignment is platform-dependent. On systems without threading support
        // (no WIN32_THREADS and no PTHREADS), num must be set to 1. The previous assignments are
        // only active on specific platforms (Linux, macOS, Windows), so this is not redundant.
        num = 1;
#endif
#endif

        // Lets limit the number of threads to XSIGMA_MAX_THREADS
        num = std::min(num, XSIGMA_MAX_THREADS);

        xsigmaMultiThreaderGlobalDefaultNumberOfThreads = num;
    }

    return xsigmaMultiThreaderGlobalDefaultNumberOfThreads;
}

// Constructor. Default all the methods to nullptr. Since the
// ThreadInfoArray is static, the ThreadIDs can be initialized here
// and will not change.
multi_threader::multi_threader()
{
    for (int i = 0; i < XSIGMA_MAX_THREADS; i++)
    {
        this->ThreadInfoArray[i].ThreadID        = i;
        this->ThreadInfoArray[i].ActiveFlag      = nullptr;
        this->ThreadInfoArray[i].ActiveFlagLock  = nullptr;
        this->MultipleMethod[i]                  = nullptr;
        this->SpawnedThreadActiveFlag[i]         = 0;
        this->SpawnedThreadActiveFlagLock[i]     = nullptr;
        this->SpawnedThreadProcessID[i]          = xsigmaThreadProcessIDType{};
        this->SpawnedThreadInfoArray[i].ThreadID = i;
        this->MultipleData[i]                    = nullptr;
    }

    this->SingleMethod    = nullptr;
    this->SingleData      = nullptr;
    this->NumberOfThreads = multi_threader::GetGlobalDefaultNumberOfThreads();
}

multi_threader::~multi_threader()
{
    for (int i = 0; i < XSIGMA_MAX_THREADS; i++)
    {
        delete this->ThreadInfoArray[i].ActiveFlagLock;
        delete this->SpawnedThreadActiveFlagLock[i];
    }
}

//----------------------------------------------------------------------------
void multi_threader::SetNumberOfThreads(int _arg)
{
    this->NumberOfThreads = std::clamp(_arg, 1, XSIGMA_MAX_THREADS);
}

//----------------------------------------------------------------------------
int multi_threader::GetNumberOfThreadsMinValue()
{
    return 1;
}

//----------------------------------------------------------------------------
int multi_threader::GetNumberOfThreadsMaxValue()
{
    return XSIGMA_MAX_THREADS;
}

//------------------------------------------------------------------------------
int multi_threader::GetNumberOfThreads()
{
    int num = this->NumberOfThreads;
    if (xsigmaMultiThreaderGlobalMaximumNumberOfThreads > 0 &&
        num > xsigmaMultiThreaderGlobalMaximumNumberOfThreads)
    {
        num = xsigmaMultiThreaderGlobalMaximumNumberOfThreads;
    }
    return num;
}

// Set the user defined method that will be run on NumberOfThreads threads
// when SingleMethodExecute is called.
void multi_threader::SetSingleMethod(xsigmaThreadFunctionType f, void* data)
{
    this->SingleMethod = f;
    this->SingleData   = data;
}

// Set one of the user defined methods that will be run on NumberOfThreads
// threads when MultipleMethodExecute is called. This method should be
// called with index = 0, 1, ..,  NumberOfThreads-1 to set up all the
// required user defined methods
void multi_threader::SetMultipleMethod(int index, xsigmaThreadFunctionType f, void* data)
{
    // You can only set the method for 0 through NumberOfThreads-1

    XSIGMA_CHECK(
        index < this->NumberOfThreads,
        "Can't set method ",
        index,
        " with a thread count of ",
        this->NumberOfThreads);

    this->MultipleMethod[index] = f;
    this->MultipleData[index]   = data;
}

// Execute the method set as the SingleMethod on NumberOfThreads threads.
void multi_threader::SingleMethodExecute()
{
#ifdef XSIGMA_HAS_WIN32_THREADS
    int    thread_loop;
    DWORD  threadId;
    HANDLE process_id[XSIGMA_MAX_THREADS] = {};  //NOLINT
#endif

#ifdef XSIGMA_HAS_PTHREADS
    pthread_t process_id[XSIGMA_MAX_THREADS] = {};  //NOLINT
#endif
    XSIGMA_CHECK(this->SingleMethod, "No single method set!");
    // obey the global maximum number of threads limit
    if ((xsigmaMultiThreaderGlobalMaximumNumberOfThreads != 0) &&
        this->NumberOfThreads > xsigmaMultiThreaderGlobalMaximumNumberOfThreads)
    {
        this->NumberOfThreads = xsigmaMultiThreaderGlobalMaximumNumberOfThreads;
    }

#ifdef XSIGMA_HAS_WIN32_THREADS
    // Using CreateThread on Windows
    //
    // We want to use CreateThread to start this->NumberOfThreads - 1
    // additional threads which will be used to call this->SingleMethod().
    // The parent thread will also call this routine.  When it is done,
    // it will wait for all the children to finish.
    //
    // First, start up the this->NumberOfThreads-1 processes.  Keep track
    // of their process ids for use later in the waitid call
    for (thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++)
    {
        this->ThreadInfoArray[thread_loop].UserData        = this->SingleData;
        this->ThreadInfoArray[thread_loop].NumberOfThreads = this->NumberOfThreads;
        process_id[thread_loop]                            = CreateThread(
            nullptr,
            0,
            this->SingleMethod,
            static_cast<void*>(&this->ThreadInfoArray[thread_loop]),
            0,
            &threadId);

        XSIGMA_CHECK(process_id[thread_loop], "Error in thread creation !!!");
    }

    // Now, the parent thread calls this->SingleMethod() itself
    this->ThreadInfoArray[0].UserData        = this->SingleData;
    this->ThreadInfoArray[0].NumberOfThreads = this->NumberOfThreads;
    this->SingleMethod(static_cast<void*>(&this->ThreadInfoArray[0]));

    // The parent thread has finished this->SingleMethod() - so now it
    // waits for each of the other processes to exit
    for (thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++)
    {
        WaitForSingleObject(process_id[thread_loop], INFINITE);
    }

    // close the threads
    for (thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++)
    {
        CloseHandle(process_id[thread_loop]);
    }
#endif

#ifdef XSIGMA_HAS_PTHREADS
    // Using POSIX threads
    //
    // We want to use pthread_create to start this->NumberOfThreads-1 additional
    // threads which will be used to call this->SingleMethod(). The
    // parent thread will also call this routine.  When it is done,
    // it will wait for all the children to finish.
    //
    // First, start up the this->NumberOfThreads-1 processes.  Keep track
    // of their process ids for use later in the pthread_join call

    int            thread_loop;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
#if !defined(__CYGWIN__) && !defined(__EMSCRIPTEN__)
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
#endif

    for (thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++)
    {
        this->ThreadInfoArray[thread_loop].UserData        = this->SingleData;
        this->ThreadInfoArray[thread_loop].NumberOfThreads = this->NumberOfThreads;

        int const threadError = pthread_create(
            &(process_id[thread_loop]),
            &attr,
            reinterpret_cast<xsigmaExternCThreadFunctionType>(this->SingleMethod),
            static_cast<void*>(&this->ThreadInfoArray[thread_loop]));

        XSIGMA_CHECK(
            threadError == 0,
            "Unable to create a thread.  pthread_create() returned ",
            threadError);
    }

    // Now, the parent thread calls this->SingleMethod() itself
    this->ThreadInfoArray[0].UserData        = this->SingleData;
    this->ThreadInfoArray[0].NumberOfThreads = this->NumberOfThreads;
    this->SingleMethod(static_cast<void*>(&this->ThreadInfoArray[0]));

    // The parent thread has finished this->SingleMethod() - so now it
    // waits for each of the other processes to exit
    for (thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++)
    {
        pthread_join(process_id[thread_loop], nullptr);
    }
#endif

#ifndef XSIGMA_HAS_WIN32_THREADS
#ifndef XSIGMA_HAS_PTHREADS
    // There is no multi threading, so there is only one thread.
    this->ThreadInfoArray[0].UserData        = this->SingleData;
    this->ThreadInfoArray[0].NumberOfThreads = this->NumberOfThreads;
    this->SingleMethod(static_cast<void*>(&this->ThreadInfoArray[0]));
#endif
#endif
}

void multi_threader::MultipleMethodExecute()
{
    int thread_loop;

#ifdef XSIGMA_HAS_WIN32_THREADS
    DWORD  threadId;
    HANDLE process_id[XSIGMA_MAX_THREADS] = {};  //NOLINT
#endif

#ifdef XSIGMA_HAS_PTHREADS
    pthread_t process_id[XSIGMA_MAX_THREADS] = {};  //NOLINT
#endif

    // obey the global maximum number of threads limit
    if ((xsigmaMultiThreaderGlobalMaximumNumberOfThreads != 0) &&
        this->NumberOfThreads > xsigmaMultiThreaderGlobalMaximumNumberOfThreads)
    {
        this->NumberOfThreads = xsigmaMultiThreaderGlobalMaximumNumberOfThreads;
    }

    for (thread_loop = 0; thread_loop < this->NumberOfThreads; thread_loop++)
    {
        XSIGMA_CHECK(
            (this->MultipleMethod[thread_loop] != static_cast<xsigmaThreadFunctionType>(nullptr)),
            "No multiple method set for: ",
            thread_loop);
    }

#ifdef XSIGMA_HAS_WIN32_THREADS
    // Using CreateThread on Windows
    //
    // We want to use CreateThread to start this->NumberOfThreads - 1
    // additional threads which will be used to call the NumberOfThreads-1
    // methods defined in this->MultipleMethods[](). The parent thread
    // will call this->MultipleMethods[NumberOfThreads-1]().  When it is done,
    // it will wait for all the children to finish.
    //
    // First, start up the this->NumberOfThreads-1 processes.  Keep track
    // of their process ids for use later in the waitid call
    for (thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++)
    {
        this->ThreadInfoArray[thread_loop].UserData        = this->MultipleData[thread_loop];
        this->ThreadInfoArray[thread_loop].NumberOfThreads = this->NumberOfThreads;
        process_id[thread_loop]                            = CreateThread(
            nullptr,
            0,
            this->MultipleMethod[thread_loop],
            static_cast<void*>(&this->ThreadInfoArray[thread_loop]),
            0,
            &threadId);

        XSIGMA_CHECK(process_id[thread_loop], "Error in thread creation !!!");
    }

    // Now, the parent thread calls the last method itself
    this->ThreadInfoArray[0].UserData        = this->MultipleData[0];
    this->ThreadInfoArray[0].NumberOfThreads = this->NumberOfThreads;
    (this->MultipleMethod[0])(static_cast<void*>(&this->ThreadInfoArray[0]));

    // The parent thread has finished its method - so now it
    // waits for each of the other threads to exit
    for (thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++)
    {
        WaitForSingleObject(process_id[thread_loop], INFINITE);
    }

    // close the threads
    for (thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++)
    {
        CloseHandle(process_id[thread_loop]);
    }
#endif

#ifdef XSIGMA_HAS_PTHREADS
    // Using POSIX threads
    //
    // We want to use pthread_create to start this->NumberOfThreads - 1
    // additional
    // threads which will be used to call the NumberOfThreads-1 methods
    // defined in this->MultipleMethods[](). The parent thread
    // will call this->MultipleMethods[NumberOfThreads-1]().  When it is done,
    // it will wait for all the children to finish.
    //
    // First, start up the this->NumberOfThreads-1 processes.  Keep track
    // of their process ids for use later in the pthread_join call

    pthread_attr_t attr;

    pthread_attr_init(&attr);
#if !defined(__CYGWIN__) && !defined(__EMSCRIPTEN__)
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
#endif

    for (thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++)
    {
        this->ThreadInfoArray[thread_loop].UserData        = this->MultipleData[thread_loop];
        this->ThreadInfoArray[thread_loop].NumberOfThreads = this->NumberOfThreads;
        pthread_create(
            &(process_id[thread_loop]),
            &attr,
            reinterpret_cast<xsigmaExternCThreadFunctionType>(this->MultipleMethod[thread_loop]),
            static_cast<void*>(&this->ThreadInfoArray[thread_loop]));
    }

    // Now, the parent thread calls the last method itself
    this->ThreadInfoArray[0].UserData        = this->MultipleData[0];
    this->ThreadInfoArray[0].NumberOfThreads = this->NumberOfThreads;
    (this->MultipleMethod[0])(static_cast<void*>(&this->ThreadInfoArray[0]));

    // The parent thread has finished its method - so now it
    // waits for each of the other processes to exit
    for (thread_loop = 1; thread_loop < this->NumberOfThreads; thread_loop++)
    {
        pthread_join(process_id[thread_loop], nullptr);
    }
#endif

#ifndef XSIGMA_HAS_WIN32_THREADS
#ifndef XSIGMA_HAS_PTHREADS
    // There is no multi threading, so there is only one thread.
    this->ThreadInfoArray[0].UserData        = this->MultipleData[0];
    this->ThreadInfoArray[0].NumberOfThreads = this->NumberOfThreads;
    (this->MultipleMethod[0])(static_cast<void*>(&this->ThreadInfoArray[0]));
#endif
#endif
}

int multi_threader::SpawnThread(const xsigmaThreadFunctionType /*f*/, void* userdata)
{
    int id;

    for (id = 0; id < XSIGMA_MAX_THREADS; id++)
    {
        if (this->SpawnedThreadActiveFlagLock[id] == nullptr)
        {
            this->SpawnedThreadActiveFlagLock[id] = new std::mutex;
        }
        XSIGMA_UNUSED std::scoped_lock const lockGuard(*this->SpawnedThreadActiveFlagLock[id]);
        if (this->SpawnedThreadActiveFlag[id] == 0)
        {
            // We've got a usable thread id, so grab it
            this->SpawnedThreadActiveFlag[id] = 1;
            break;
        }
    }

    XSIGMA_CHECK(id < XSIGMA_MAX_THREADS, "You have too many active threads!");

    this->SpawnedThreadInfoArray[id].UserData        = userdata;
    this->SpawnedThreadInfoArray[id].NumberOfThreads = 1;
    this->SpawnedThreadInfoArray[id].ActiveFlag      = &this->SpawnedThreadActiveFlag[id];
    this->SpawnedThreadInfoArray[id].ActiveFlagLock  = this->SpawnedThreadActiveFlagLock[id];

#ifdef XSIGMA_HAS_WIN32_THREADS
    // Using CreateThread on Windows
    //
    DWORD threadId;
    this->SpawnedThreadProcessID[id] = CreateThread(
        nullptr, 0, f, static_cast<void*>(&this->SpawnedThreadInfoArray[id]), 0, &threadId);
    XSIGMA_CHECK(this->SpawnedThreadProcessID[id], "Error in thread creation !!!");
#endif

#ifdef XSIGMA_HAS_PTHREADS
    // Using POSIX threads
    //
    pthread_attr_t attr;
    pthread_attr_init(&attr);
#if !defined(__CYGWIN__) && !defined(__EMSCRIPTEN__)
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
#endif

    pthread_create(
        &(this->SpawnedThreadProcessID[id]),
        &attr,
        reinterpret_cast<xsigmaExternCThreadFunctionType>(f),
        static_cast<void*>(&this->SpawnedThreadInfoArray[id]));

#endif

#ifndef XSIGMA_HAS_WIN32_THREADS
#ifndef XSIGMA_HAS_PTHREADS
    // There is no multi threading, so there is only one thread.
    // This won't work - so give an error message.
    XSIGMA_THROW("Cannot spawn thread in a single threaded environment!");
    delete this->SpawnedThreadActiveFlagLock[id];
    id = -1;
#endif
#endif

    return id;
}

void multi_threader::TerminateThread(int threadId)
{
    // check if the threadId argument is in range
    XSIGMA_CHECK(
        threadId < XSIGMA_MAX_THREADS,
        "threadId is out of range. Must be less that ",
        XSIGMA_MAX_THREADS);

    // If we don't have a lock, then this thread is definitely not active
    if (this->SpawnedThreadActiveFlag[threadId] == 0)
    {
        return;
    }

    // If we do have a lock, use it and find out the status of the active flag
    int val = 0;
    {
        XSIGMA_UNUSED std::scoped_lock const lockGuard(
            *this->SpawnedThreadActiveFlagLock[threadId]);
        val = this->SpawnedThreadActiveFlag[threadId];
    }

    // If the active flag is 0, return since this thread is not active
    if (val == 0)
    {
        return;
    }

    // OK - now we know we have an active thread - set the active flag to 0
    // to indicate to the thread that it should terminate itself
    {
        XSIGMA_UNUSED std::scoped_lock const lockGuard(
            *this->SpawnedThreadActiveFlagLock[threadId]);
        this->SpawnedThreadActiveFlag[threadId] = 0;
    }

#ifdef XSIGMA_HAS_WIN32_THREADS
    WaitForSingleObject(this->SpawnedThreadProcessID[threadId], INFINITE);
    CloseHandle(this->SpawnedThreadProcessID[threadId]);
#endif

#ifdef XSIGMA_HAS_PTHREADS
    pthread_join(this->SpawnedThreadProcessID[threadId], nullptr);
#endif

#ifndef XSIGMA_HAS_WIN32_THREADS
#ifndef XSIGMA_HAS_PTHREADS
    // There is no multi threading, so there is only one thread.
    // This won't work - so give an error message.
    XSIGMA_THROW("Cannot terminate thread in single threaded environment!");
#endif
#endif

    delete this->SpawnedThreadActiveFlagLock[threadId];
    this->SpawnedThreadActiveFlagLock[threadId] = nullptr;
}

//------------------------------------------------------------------------------
xsigmaMultiThreaderIDType multi_threader::GetCurrentThreadID()
{
#ifdef XSIGMA_HAS_PTHREADS
    return pthread_self();
#elif defined(XSIGMA_HAS_WIN32_THREADS)
    return GetCurrentThreadId();
#else
    // No threading implementation.  Assume all callers are in the same
    // thread.
    return 0;
#endif
}

bool multi_threader::IsThreadActive(int threadId)
{
    // check if the threadId argument is in range
    XSIGMA_CHECK(
        threadId < XSIGMA_MAX_THREADS,
        "threadId is out of range. Must be less that ",
        XSIGMA_MAX_THREADS);

    // If we don't have a lock, then this thread is not active
    if (this->SpawnedThreadActiveFlagLock[threadId] == nullptr)
    {
        return false;
    }

    // We have a lock - use it to get the active flag value
    int val = 0;
    {
        XSIGMA_UNUSED std::scoped_lock const lockGuard(
            *this->SpawnedThreadActiveFlagLock[threadId]);
        val = this->SpawnedThreadActiveFlag[threadId];
    }

    // now return that value
    return val == 1;
}

//------------------------------------------------------------------------------
bool multi_threader::ThreadsEqual(
    xsigmaMultiThreaderIDType /*t1*/, xsigmaMultiThreaderIDType /*t2*/)
{
#ifdef XSIGMA_HAS_PTHREADS
    return pthread_equal(t1, t2) != 0;
#elif defined(XSIGMA_HAS_WIN32_THREADS)
    return t1 == t2;
#else
    // No threading implementation.  Assume all callers are in the same
    // thread.
    return 1;
#endif
}
}  // namespace xsigma
