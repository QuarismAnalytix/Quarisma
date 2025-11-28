
// This file incorporates code from the Visualization Toolkit (VTK) and remains subject to the BSD-3-Clause VTK license.

/**
 * @class   multi_threader
 * @brief   A class for performing multithreaded execution
 *
 * multi_threader is a class that provides support for multithreaded
 * execution using pthreads on POSIX systems, or Win32 threads on
 * Windows. This class can be used to execute a single
 * method on multiple threads, or to specify a method per thread.
 *
 * This class serves as the foundational threading infrastructure for the XSIGMA
 * library, providing a unified interface for multi-threaded operations across
 * different platforms. It abstracts away the platform-specific threading
 * implementations (pthreads, Win32 threads) and provides a consistent API.
 *
 * The class supports two main modes of operation:
 * 1. Single Method Execution - where the same function runs across multiple threads
 * 2. Multiple Method Execution - where different functions can be assigned to different threads
 *
 * Additionally, it provides functionality to spawn individual threads outside of
 * the main thread pool, useful for background processing tasks.
 *
 * Thread management is handled internally, including thread creation, execution,
 * and termination. The class also provides utilities for controlling the number of
 * threads and handling thread priorities.
 */

#pragma once

#include <array>  // for array
#include <mutex>  // for mutex

#include "common/macros.h"
#include "common/macros.h"  // for XSIGMA_DELETE_COPY

#if XSIGMA_HAS_PTHREADS
#include <pthread.h>    // Needed for PTHREAD implementation of mutex
#include <sys/types.h>  // Needed for unix implementation of pthreads
#include <unistd.h>     // Needed for unix implementation of pthreads
#endif

// If XSIGMA_HAS_PTHREADS is defined, then pthread_create() will be
// used to create multiple threads

// If XSIGMA_HAS_PTHREADS is defined, then the multithreaded
// function is of type void *, and returns nullptr
// Otherwise the type is void which is correct for WIN32

//   XSIGMA_MAX_THREADS - Maximum number of threads supported
//   __XSIGMA_THREAD_RETURN_VALUE__ - Return value for thread functions
//   __XSIGMA_THREAD_RETURN_TYPE__ - Return type for thread functions

// Define platform-specific thread function types and thread ID types
#if XSIGMA_HAS_PTHREADS
typedef void* (*xsigmaThreadFunctionType)(void*);  // NOLINT
typedef pthread_t xsigmaThreadProcessIDType;
// #define __XSIGMA_THREAD_RETURN_VALUE__  nullptr
// #define __XSIGMA_THREAD_RETURN_TYPE__   void *
typedef pthread_t xsigmaMultiThreaderIDType;
#endif

#if XSIGMA_HAS_WIN32_THREADS

#if defined(_WIN32)
#include <windows.h>
// Define types from the windows header file.
using xsigmaWindowsDWORD                  = DWORD;
using xsigmaWindowsPVOID                  = PVOID;
using xsigmaWindowsLPVOID                 = LPVOID;
using xsigmaWindowsHANDLE                 = HANDLE;
using xsigmaWindowsLPTHREAD_START_ROUTINE = LPTHREAD_START_ROUTINE;
#else
// Define types from the windows header file.
using xsigmaWindowsDWORD                  = unsigned long;
using xsigmaWindowsPVOID                  = void*;
using xsigmaWindowsLPVOID                 = xsigmaWindowsPVOID;
using xsigmaWindowsHANDLE                 = xsigmaWindowsPVOID;
using xsigmaWindowsLPTHREAD_START_ROUTINE = xsigmaWindowsDWORD(__stdcall*)(xsigmaWindowsLPVOID);
#endif

using xsigmaThreadFunctionType  = xsigmaWindowsLPTHREAD_START_ROUTINE;
using xsigmaThreadProcessIDType = xsigmaWindowsHANDLE;
// #define __XSIGMA_THREAD_RETURN_VALUE__ 0
// #define __XSIGMA_THREAD_RETURN_TYPE__ DWORD __stdcall
using xsigmaMultiThreaderIDType = xsigmaWindowsDWORD;
#endif

// Fallback definitions for platforms without specific threading support
#if !XSIGMA_HAS_PTHREADS && !XSIGMA_HAS_WIN32_THREADS
using xsigmaThreadFunctionType  = void (*)(void*);
using xsigmaThreadProcessIDType = int;
// #define __XSIGMA_THREAD_RETURN_VALUE__
// #define __XSIGMA_THREAD_RETURN_TYPE__ void
using xsigmaMultiThreaderIDType = int;
#endif

namespace xsigma
{
/**
 * @brief Main multi-threading utility class for cross-platform thread management
 *
 * This class provides a uniform interface for thread management across
 * different platforms (Windows, POSIX). It supports various threading models
 * including parallel execution of a single method, concurrent execution of
 * multiple methods, and individual thread spawning.
 */
class XSIGMA_VISIBILITY multi_threader
{
public:
    /**
     * @brief Default constructor
     *
     * Initializes a multi_threader instance with the default number of threads
     * based on system capabilities and global configuration.
     */
    multi_threader();

    /**
     * @brief Virtual destructor
     *
     * Cleans up all thread-related resources including mutexes and active flags.
     */
    virtual ~multi_threader();

    /**
     * @brief Thread information structure passed to thread functions
     *
     * This structure contains information about the thread context including:
     * - ThreadID: A unique identifier for the thread (0 to NumberOfThreads-1)
     * - NumberOfThreads: Total number of threads in the pool
     * - ActiveFlag: Flag to indicate if the thread should continue running
     * - ActiveFlagLock: Mutex to protect the active flag
     * - UserData: User-provided data to be passed to the thread function
     */
    class ThreadInfo
    {
    public:
        int         ThreadID;         // Unique ID for this thread (0 to NumberOfThreads-1)
        int         NumberOfThreads;  // Total number of threads in use
        int*        ActiveFlag;       // Flag indicating if thread should continue running
        std::mutex* ActiveFlagLock;   // Mutex to protect the ActiveFlag
        void*       UserData;         // User data passed to the thread function
    };

    ///@{
    /**
     * @brief Set the number of threads to use for parallel operations
     *
     * The number will be clamped to the range 1 - XSIGMA_MAX_THREADS.
     * Use GetNumberOfThreads() after setting to confirm the actual value.
     *
     * @param _arg Desired number of threads
     */
    void SetNumberOfThreads(int _arg);

    /**
     * @brief Get the minimum allowed value for number of threads (always 1)
     * @return Minimum thread count (1)
     */
    static int GetNumberOfThreadsMinValue();

    /**
     * @brief Get the maximum allowed value for number of threads
     * @return Maximum thread count (defined by XSIGMA_MAX_THREADS)
     */
    static int GetNumberOfThreadsMaxValue();

    /**
     * @brief Get the effective number of threads that will be used
     *
     * This returns the actual number of threads that will be used, which
     * may be less than the set number if the global maximum is lower.
     *
     * @return Effective number of threads
     */
    virtual int GetNumberOfThreads();
    ///@}

    ///@{
    /**
     * @brief Get the maximum number of threads supported by the system
     *
     * This is the absolute upper limit on threads, as defined by
     * XSIGMA_MAX_THREADS in the configuration.
     *
     * @return Maximum supported thread count
     */
    static int GetGlobalStaticMaximumNumberOfThreads();
    ///@}

    ///@{
    /**
     * @brief Set/Get the global maximum number of threads to use
     *
     * This setting affects all multi_threader instances in the application.
     * A value of zero indicates no limit (uses the static maximum).
     *
     * @param val Maximum number of threads to use globally
     * @return Current global maximum thread count
     */
    static void SetGlobalMaximumNumberOfThreads(int val);
    static int  GetGlobalMaximumNumberOfThreads();
    ///@}

    ///@{
    /**
     * @brief Set/Get the default number of threads for new instances
     *
     * This value is used to initialize the NumberOfThreads in the constructor.
     * By default, it's set to the number of processors or XSIGMA_MAX_THREADS,
     * whichever is less.
     *
     * @param val Default number of threads for new instances
     * @return Current default thread count
     */
    static void SetGlobalDefaultNumberOfThreads(int val);
    static int  GetGlobalDefaultNumberOfThreads();
    ///@}

    /**
     * @brief Execute a single method across multiple threads
     *
     * Executes the method specified by SetSingleMethod() on this->NumberOfThreads
     * threads. Each thread receives a ThreadInfo structure with a unique ThreadID.
     */
    void SingleMethodExecute();

    /**
     * @brief Execute multiple methods concurrently
     *
     * Executes the methods set with SetMultipleMethod() on this->NumberOfThreads
     * threads. Each method is executed on its corresponding thread.
     */
    void MultipleMethodExecute();

    /**
     * @brief Set the single method to execute across all threads
     *
     * @param f Function pointer to execute (must be of type xsigmaThreadFunctionType)
     * @param data User data to pass to each thread (available in ThreadInfo)
     */
    void SetSingleMethod(xsigmaThreadFunctionType f, void* data);

    /**
     * @brief Set a specific method for a particular thread
     *
     * @param index Thread index (0 to NumberOfThreads-1)
     * @param f Function pointer to execute on the specified thread
     * @param data User data to pass to the thread
     */
    void SetMultipleMethod(int index, xsigmaThreadFunctionType f, void* data);

    /**
     * @brief Create a new thread for a specific function
     *
     * Creates a new thread outside the main thread pool. Returns a thread ID
     * which can be used to terminate the thread later.
     *
     * @param f Function to execute in the new thread
     * @param data User data to pass to the thread
     * @return Thread ID for the new thread (0 to XSIGMA_MAX_THREADS-1)
     */
    int SpawnThread(xsigmaThreadFunctionType f, void* data);

    /**
     * @brief Terminate a thread created with SpawnThread
     *
     * @param threadId Thread ID returned by SpawnThread
     */
    void TerminateThread(int threadId);

    /**
     * @brief Check if a spawned thread is still active
     *
     * @param threadId Thread ID to check
     * @return true if the thread is still running, false otherwise
     */
    bool IsThreadActive(int threadId);

    /**
     * @brief Get the thread identifier of the calling thread
     *
     * @return Platform-specific thread ID of the current thread
     */
    static xsigmaMultiThreaderIDType GetCurrentThreadID();

    /**
     * @brief Compare two thread identifiers for equality
     *
     * @param t1 First thread ID
     * @param t2 Second thread ID
     * @return true if the thread IDs refer to the same thread
     */
    static bool ThreadsEqual(xsigmaMultiThreaderIDType t1, xsigmaMultiThreaderIDType t2);

protected:
    int NumberOfThreads;  // The number of threads to use for parallel operations

    // Thread information array for each thread in the pool
    // Contains thread ID, count, and user data pointer
    ThreadInfo ThreadInfoArray[XSIGMA_MAX_THREADS];

    // Function pointers for thread methods
    xsigmaThreadFunctionType SingleMethod;  // Function for single method execution
    xsigmaThreadFunctionType
        MultipleMethod[XSIGMA_MAX_THREADS];  // Functions for multiple method execution

    // Storage for spawned threads
    int                       SpawnedThreadActiveFlag[XSIGMA_MAX_THREADS];      // Activity flags
    std::mutex*               SpawnedThreadActiveFlagLock[XSIGMA_MAX_THREADS];  // Mutex locks
    xsigmaThreadProcessIDType SpawnedThreadProcessID[XSIGMA_MAX_THREADS];  // Thread process IDs
    ThreadInfo                SpawnedThreadInfoArray[XSIGMA_MAX_THREADS];  // Thread info structures

    // User data storage
    void* SingleData;                        // Data for single method execution
    void* MultipleData[XSIGMA_MAX_THREADS];  // Data for multiple method execution

private:
    // Deleted copy constructor and assignment operator to prevent copying
    multi_threader(const multi_threader&) = delete;
    void operator=(const multi_threader&) = delete;
};

// Type alias for ThreadInfo structure for backward compatibility
using ThreadInfoStruct = multi_threader::ThreadInfo;

}  // namespace xsigma
