// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "multi_threader.h"

#ifdef _WIN32
#include <windows.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include <algorithm>

// Need to define "extern_c_thread_function_type" to avoid warning on some
// platforms about passing function pointer to an argument expecting an
// extern "C" function.  Placing the typedef of the function pointer type
// inside an extern "C" block solves this problem.
#if defined(CONDUCTOR_USE_PTHREADS)
#include <pthread.h>
extern "C"
{
  typedef void* (*extern_c_thread_function_type)(void*);
}
#else
typedef thread_function_type extern_c_thread_function_type;
#endif

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <sys/types.h>
#endif

// Initialize static member that controls global maximum number of threads
static int g_multi_threader_global_maximum_number_of_threads = 0;

void multi_threader::set_global_maximum_number_of_threads(int val)
{
  if (val == g_multi_threader_global_maximum_number_of_threads)
  {
    return;
  }
  g_multi_threader_global_maximum_number_of_threads = val;
}

int multi_threader::get_global_maximum_number_of_threads()
{
  return g_multi_threader_global_maximum_number_of_threads;
}

int multi_threader::get_global_static_maximum_number_of_threads()
{
  return CONDUCTOR_MAX_THREADS;
}

// 0 => Not initialized.
static int g_multi_threader_global_default_number_of_threads = 0;

void multi_threader::set_global_default_number_of_threads(int val)
{
  if (val == g_multi_threader_global_default_number_of_threads)
  {
    return;
  }
  g_multi_threader_global_default_number_of_threads = val;
}

int multi_threader::get_global_default_number_of_threads()
{
  if (g_multi_threader_global_default_number_of_threads == 0)
  {
    int num = 1; // default is 1

#ifdef CONDUCTOR_USE_PTHREADS
    // Default the number of threads to be the number of available
    // processors if we are using pthreads()
#ifdef _SC_NPROCESSORS_ONLN
    num = sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(_SC_NPROC_ONLN)
    num = sysconf(_SC_NPROC_ONLN);
#endif
#endif

#ifdef __APPLE__
    // Determine the number of CPU cores.
    // hw.logicalcpu takes into account cores/CPUs that are
    // disabled because of power management.
    size_t dataLen = sizeof(int); // 'num' is an 'int'
    int result = sysctlbyname("hw.logicalcpu", &num, &dataLen, nullptr, 0);
    if (result == -1)
    {
      num = 1;
    }
#endif

#ifdef _WIN32
    {
      SYSTEM_INFO sysInfo;
      GetSystemInfo(&sysInfo);
      num = sysInfo.dwNumberOfProcessors;
    }
#endif

#ifndef CONDUCTOR_USE_WIN32_THREADS
#ifndef CONDUCTOR_USE_PTHREADS
    // If we are not multithreading, the number of threads should
    // always be 1
    num = 1;
#endif
#endif

    // Lets limit the number of threads to CONDUCTOR_MAX_THREADS
    num = std::min(num, CONDUCTOR_MAX_THREADS);

    g_multi_threader_global_default_number_of_threads = num;
  }

  return g_multi_threader_global_default_number_of_threads;
}

// Constructor. Default all the methods to nullptr. Since the
// thread_info_array is static, the thread_ids can be initialized here
// and will not change.
multi_threader::multi_threader()
{
  for (int i = 0; i < CONDUCTOR_MAX_THREADS; i++)
  {
    m_thread_info_array[i].thread_id = i;
    m_thread_info_array[i].active_flag = nullptr;
    m_thread_info_array[i].active_flag_lock = nullptr;
    m_multiple_method[i] = nullptr;
    m_spawned_thread_active_flag[i] = 0;
    m_spawned_thread_active_flag_lock[i] = nullptr;
    m_spawned_thread_info_array[i].thread_id = i;
  }

  m_single_method = nullptr;
  m_number_of_threads = multi_threader::get_global_default_number_of_threads();
}

multi_threader::~multi_threader()
{
  for (int i = 0; i < CONDUCTOR_MAX_THREADS; i++)
  {
    delete m_thread_info_array[i].active_flag_lock;
    delete m_spawned_thread_active_flag_lock[i];
  }
}

multi_threader* multi_threader::create()
{
  return new multi_threader();
}

//------------------------------------------------------------------------------
int multi_threader::get_number_of_threads()
{
  int num = m_number_of_threads;
  if (g_multi_threader_global_maximum_number_of_threads > 0 &&
    num > g_multi_threader_global_maximum_number_of_threads)
  {
    num = g_multi_threader_global_maximum_number_of_threads;
  }
  return num;
}

void multi_threader::set_number_of_threads(int num)
{
  if (num < 1)
  {
    num = 1;
  }
  if (num > CONDUCTOR_MAX_THREADS)
  {
    num = CONDUCTOR_MAX_THREADS;
  }
  m_number_of_threads = num;
}

// Set the user defined method that will be run on number_of_threads threads
// when single_method_execute is called.
void multi_threader::set_single_method(thread_function_type f, void* data)
{
  m_single_method = f;
  m_single_data = data;
}

// Set one of the user defined methods that will be run on number_of_threads
// threads when multiple_method_execute is called. This method should be
// called with index = 0, 1, ..,  number_of_threads-1 to set up all the
// required user defined methods
void multi_threader::set_multiple_method(int index, thread_function_type f, void* data)
{
  // You can only set the method for 0 through number_of_threads-1
  if (index >= m_number_of_threads)
  {
    // Error: Can't set method with index >= number_of_threads
    return;
  }
  else
  {
    m_multiple_method[index] = f;
    m_multiple_data[index] = data;
  }
}

// Execute the method set as the single_method on number_of_threads threads.
void multi_threader::single_method_execute()
{
  int thread_loop = 0;

#ifdef CONDUCTOR_USE_WIN32_THREADS
  DWORD threadId;
  HANDLE process_id[CONDUCTOR_MAX_THREADS] = {};
#endif

#ifdef CONDUCTOR_USE_PTHREADS
  pthread_t process_id[CONDUCTOR_MAX_THREADS] = {};
#endif

  if (!m_single_method)
  {
    // Error: No single method set!
    return;
  }

  // obey the global maximum number of threads limit
  if (g_multi_threader_global_maximum_number_of_threads &&
    m_number_of_threads > g_multi_threader_global_maximum_number_of_threads)
  {
    m_number_of_threads = g_multi_threader_global_maximum_number_of_threads;
  }

#ifdef CONDUCTOR_USE_WIN32_THREADS
  // Using CreateThread on Windows
  //
  // We want to use CreateThread to start m_number_of_threads - 1
  // additional threads which will be used to call m_single_method().
  // The parent thread will also call this routine.  When it is done,
  // it will wait for all the children to finish.
  //
  // First, start up the m_number_of_threads-1 processes.  Keep track
  // of their process ids for use later in the waitid call
  for (thread_loop = 1; thread_loop < m_number_of_threads; thread_loop++)
  {
    m_thread_info_array[thread_loop].user_data = m_single_data;
    m_thread_info_array[thread_loop].number_of_threads = m_number_of_threads;
    process_id[thread_loop] = CreateThread(
      nullptr, 0, m_single_method, ((void*)(&m_thread_info_array[thread_loop])), 0, &threadId);
    if (process_id[thread_loop] == nullptr)
    {
      // Error in thread creation !!!
    }
  }

  // Now, the parent thread calls m_single_method() itself
  m_thread_info_array[0].user_data = m_single_data;
  m_thread_info_array[0].number_of_threads = m_number_of_threads;
  m_single_method((void*)(&m_thread_info_array[0]));

  // The parent thread has finished m_single_method() - so now it
  // waits for each of the other processes to exit
  for (thread_loop = 1; thread_loop < m_number_of_threads; thread_loop++)
  {
    WaitForSingleObject(process_id[thread_loop], INFINITE);
  }

  // close the threads
  for (thread_loop = 1; thread_loop < m_number_of_threads; thread_loop++)
  {
    CloseHandle(process_id[thread_loop]);
  }
#endif

#ifdef CONDUCTOR_USE_PTHREADS
  // Using POSIX threads
  //
  // We want to use pthread_create to start m_number_of_threads-1 additional
  // threads which will be used to call m_single_method(). The
  // parent thread will also call this routine.  When it is done,
  // it will wait for all the children to finish.
  //
  // First, start up the m_number_of_threads-1 processes.  Keep track
  // of their process ids for use later in the pthread_join call

  pthread_attr_t attr;

  pthread_attr_init(&attr);
#if !defined(__CYGWIN__) && !defined(__EMSCRIPTEN__)
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
#endif

  for (thread_loop = 1; thread_loop < m_number_of_threads; thread_loop++)
  {
    m_thread_info_array[thread_loop].user_data = m_single_data;
    m_thread_info_array[thread_loop].number_of_threads = m_number_of_threads;

    int threadError = pthread_create(&(process_id[thread_loop]), &attr,
      reinterpret_cast<extern_c_thread_function_type>(m_single_method),
      ((void*)(&m_thread_info_array[thread_loop])));
    if (threadError != 0)
    {
      // Error: Unable to create a thread
    }
  }

  // Now, the parent thread calls m_single_method() itself
  m_thread_info_array[0].user_data = m_single_data;
  m_thread_info_array[0].number_of_threads = m_number_of_threads;
  m_single_method((void*)(&m_thread_info_array[0]));

  // The parent thread has finished m_single_method() - so now it
  // waits for each of the other processes to exit
  for (thread_loop = 1; thread_loop < m_number_of_threads; thread_loop++)
  {
    pthread_join(process_id[thread_loop], nullptr);
  }
#endif

#ifndef CONDUCTOR_USE_WIN32_THREADS
#ifndef CONDUCTOR_USE_PTHREADS
  (void)thread_loop;
  // There is no multi threading, so there is only one thread.
  m_thread_info_array[0].user_data = m_single_data;
  m_thread_info_array[0].number_of_threads = m_number_of_threads;
  m_single_method((void*)(&m_thread_info_array[0]));
#endif
#endif
}

void multi_threader::multiple_method_execute()
{
  int thread_loop;

#ifdef CONDUCTOR_USE_WIN32_THREADS
  DWORD threadId;
  HANDLE process_id[CONDUCTOR_MAX_THREADS] = {};
#endif

#ifdef CONDUCTOR_USE_PTHREADS
  pthread_t process_id[CONDUCTOR_MAX_THREADS] = {};
#endif

  // obey the global maximum number of threads limit
  if (g_multi_threader_global_maximum_number_of_threads &&
    m_number_of_threads > g_multi_threader_global_maximum_number_of_threads)
  {
    m_number_of_threads = g_multi_threader_global_maximum_number_of_threads;
  }

  for (thread_loop = 0; thread_loop < m_number_of_threads; thread_loop++)
  {
    if (m_multiple_method[thread_loop] == (thread_function_type) nullptr)
    {
      // Error: No multiple method set
      return;
    }
  }

#ifdef CONDUCTOR_USE_WIN32_THREADS
  // Using CreateThread on Windows
  for (thread_loop = 1; thread_loop < m_number_of_threads; thread_loop++)
  {
    m_thread_info_array[thread_loop].user_data = m_multiple_data[thread_loop];
    m_thread_info_array[thread_loop].number_of_threads = m_number_of_threads;
    process_id[thread_loop] = CreateThread(nullptr, 0, m_multiple_method[thread_loop],
      ((void*)(&m_thread_info_array[thread_loop])), 0, &threadId);
    if (process_id[thread_loop] == nullptr)
    {
      // Error in thread creation !!!
    }
  }

  // Now, the parent thread calls the last method itself
  m_thread_info_array[0].user_data = m_multiple_data[0];
  m_thread_info_array[0].number_of_threads = m_number_of_threads;
  (m_multiple_method[0])((void*)(&m_thread_info_array[0]));

  // The parent thread has finished its method - so now it
  // waits for each of the other threads to exit
  for (thread_loop = 1; thread_loop < m_number_of_threads; thread_loop++)
  {
    WaitForSingleObject(process_id[thread_loop], INFINITE);
  }

  // close the threads
  for (thread_loop = 1; thread_loop < m_number_of_threads; thread_loop++)
  {
    CloseHandle(process_id[thread_loop]);
  }
#endif

#ifdef CONDUCTOR_USE_PTHREADS
  // Using POSIX threads
  pthread_attr_t attr;

  pthread_attr_init(&attr);
#if !defined(__CYGWIN__) && !defined(__EMSCRIPTEN__)
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
#endif

  for (thread_loop = 1; thread_loop < m_number_of_threads; thread_loop++)
  {
    m_thread_info_array[thread_loop].user_data = m_multiple_data[thread_loop];
    m_thread_info_array[thread_loop].number_of_threads = m_number_of_threads;
    pthread_create(&(process_id[thread_loop]), &attr,
      reinterpret_cast<extern_c_thread_function_type>(m_multiple_method[thread_loop]),
      ((void*)(&m_thread_info_array[thread_loop])));
  }

  // Now, the parent thread calls the last method itself
  m_thread_info_array[0].user_data = m_multiple_data[0];
  m_thread_info_array[0].number_of_threads = m_number_of_threads;
  (m_multiple_method[0])((void*)(&m_thread_info_array[0]));

  // The parent thread has finished its method - so now it
  // waits for each of the other processes to exit
  for (thread_loop = 1; thread_loop < m_number_of_threads; thread_loop++)
  {
    pthread_join(process_id[thread_loop], nullptr);
  }
#endif

#ifndef CONDUCTOR_USE_WIN32_THREADS
#ifndef CONDUCTOR_USE_PTHREADS
  // There is no multi threading, so there is only one thread.
  m_thread_info_array[0].user_data = m_multiple_data[0];
  m_thread_info_array[0].number_of_threads = m_number_of_threads;
  (m_multiple_method[0])((void*)(&m_thread_info_array[0]));
#endif
#endif
}

int multi_threader::spawn_thread(thread_function_type f, void* userdata)
{
  int id;

  for (id = 0; id < CONDUCTOR_MAX_THREADS; id++)
  {
    if (m_spawned_thread_active_flag_lock[id] == nullptr)
    {
      m_spawned_thread_active_flag_lock[id] = new std::mutex;
    }
    std::lock_guard<std::mutex> lockGuard(*m_spawned_thread_active_flag_lock[id]);
    if (m_spawned_thread_active_flag[id] == 0)
    {
      // We've got a usable thread id, so grab it
      m_spawned_thread_active_flag[id] = 1;
      break;
    }
  }

  if (id >= CONDUCTOR_MAX_THREADS)
  {
    // Error: You have too many active threads!
    return -1;
  }

  m_spawned_thread_info_array[id].user_data = userdata;
  m_spawned_thread_info_array[id].number_of_threads = 1;
  m_spawned_thread_info_array[id].active_flag = &m_spawned_thread_active_flag[id];
  m_spawned_thread_info_array[id].active_flag_lock = m_spawned_thread_active_flag_lock[id];

#ifdef CONDUCTOR_USE_WIN32_THREADS
  // Using CreateThread on Windows
  //
  DWORD threadId;
  m_spawned_thread_process_id[id] =
    CreateThread(nullptr, 0, f, ((void*)(&m_spawned_thread_info_array[id])), 0, &threadId);
  if (m_spawned_thread_process_id[id] == nullptr)
  {
    // Error in thread creation !!!
  }
#endif

#ifdef CONDUCTOR_USE_PTHREADS
  // Using POSIX threads
  //
  pthread_attr_t attr;
  pthread_attr_init(&attr);
#if !defined(__CYGWIN__) && !defined(__EMSCRIPTEN__)
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
#endif

  pthread_create(&(m_spawned_thread_process_id[id]), &attr,
    reinterpret_cast<extern_c_thread_function_type>(f),
    ((void*)(&m_spawned_thread_info_array[id])));

#endif

#ifndef CONDUCTOR_USE_WIN32_THREADS
#ifndef CONDUCTOR_USE_PTHREADS
  (void)f;
  // There is no multi threading, so there is only one thread.
  // This won't work - so give an error message.
  delete m_spawned_thread_active_flag_lock[id];
  id = -1;
#endif
#endif

  return id;
}

void multi_threader::terminate_thread(int thread_id)
{
  // check if the thread_id argument is in range
  if (thread_id >= CONDUCTOR_MAX_THREADS)
  {
    // Error: thread_id is out of range
    return;
  }

  // If we don't have a lock, then this thread is definitely not active
  if (!m_spawned_thread_active_flag[thread_id])
  {
    return;
  }

  // If we do have a lock, use it and find out the status of the active flag
  int val = 0;
  {
    std::lock_guard<std::mutex> lockGuard(*m_spawned_thread_active_flag_lock[thread_id]);
    val = m_spawned_thread_active_flag[thread_id];
  }

  // If the active flag is 0, return since this thread is not active
  if (val == 0)
  {
    return;
  }

  // OK - now we know we have an active thread - set the active flag to 0
  // to indicate to the thread that it should terminate itself
  {
    std::lock_guard<std::mutex> lockGuard(*m_spawned_thread_active_flag_lock[thread_id]);
    m_spawned_thread_active_flag[thread_id] = 0;
  }

#ifdef CONDUCTOR_USE_WIN32_THREADS
  WaitForSingleObject(m_spawned_thread_process_id[thread_id], INFINITE);
  CloseHandle(m_spawned_thread_process_id[thread_id]);
#endif

#ifdef CONDUCTOR_USE_PTHREADS
  pthread_join(m_spawned_thread_process_id[thread_id], nullptr);
#endif

#ifndef CONDUCTOR_USE_WIN32_THREADS
#ifndef CONDUCTOR_USE_PTHREADS
  // There is no multi threading, so there is only one thread.
  // This won't work - so give an error message.
#endif
#endif

  delete m_spawned_thread_active_flag_lock[thread_id];
  m_spawned_thread_active_flag_lock[thread_id] = nullptr;
}

//------------------------------------------------------------------------------
multi_threader_id_type multi_threader::get_current_thread_id()
{
#if defined(CONDUCTOR_USE_PTHREADS)
  return pthread_self();
#elif defined(CONDUCTOR_USE_WIN32_THREADS)
  return GetCurrentThreadId();
#else
  // No threading implementation.  Assume all callers are in the same
  // thread.
  return 0;
#endif
}

bool multi_threader::is_thread_active(int thread_id)
{
  // check if the thread_id argument is in range
  if (thread_id >= CONDUCTOR_MAX_THREADS)
  {
    // Error: thread_id is out of range
    return false;
  }

  // If we don't have a lock, then this thread is not active
  if (m_spawned_thread_active_flag_lock[thread_id] == nullptr)
  {
    return false;
  }

  // We have a lock - use it to get the active flag value
  int val = 0;
  {
    std::lock_guard<std::mutex> lockGuard(*m_spawned_thread_active_flag_lock[thread_id]);
    val = m_spawned_thread_active_flag[thread_id];
  }

  // now return that value
  return val != 0;
}

//------------------------------------------------------------------------------
bool multi_threader::threads_equal(multi_threader_id_type t1, multi_threader_id_type t2)
{
#if defined(CONDUCTOR_USE_PTHREADS)
  return pthread_equal(t1, t2) != 0;
#elif defined(CONDUCTOR_USE_WIN32_THREADS)
  return t1 == t2;
#else
  (void)t1;
  (void)t2;
  // No threading implementation.  Assume all callers are in the same
  // thread.
  return true;
#endif
}
