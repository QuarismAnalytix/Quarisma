// spdx-file_copyright_text: Copyright (c) T. Bellaj
// spdx-License-Identifier: bsd-3-Clause
// This file incorporates code from the Visualization Toolkit (VTK) and remains subject to the BSD-3-Clause VTK license.

#pragma once
/**
 * @file threaded_task_queue.hxx
 * @brief Task queue implementation for concurrent task execution
 *
 * This file implements a thread-safe task queue for executing functions
 * concurrently across multiple threads. It provides a high-level abstraction
 * for parallel task processing with support for different thread pool backends.
 */

#include <algorithm>  // for std::min
#include <cassert>
#include <deque>   // for std::deque
#include <future>  // for std::future, std::promise
#include <memory>  // for std::shared_ptr, std::make_shared
#include <queue>
#include <thread>  // for std::thread
#include <utility>
#include <vector>  // for std::vector

#include "common/future_extract.h"  // for future_extract
#include "smp/engine_facade.hxx"    // for TaskQueue
#include "smp/multi_threader.h"
#include "smp/thread_count.h"  // for GetGlobalDefaultNumberOfThreads
#include "logging/logger.h"

//=============================================================================
namespace xsigma
{
/**
 * @class task_queue
 * @brief Internal task queue implementation that manages function objects
 *
 * This class provides a thread-safe queue implementation for storing task functions
 * and their associated task IDs. It supports operations for pushing tasks to the queue
 * and popping tasks for execution by worker threads.
 *
 * @tparam R The return type of the functions stored in the queue
 */
template <typename R>
class task_queue
{
public:
    /**
     * @brief Constructs a task queue with the specified buffer size
     *
     * @param buffer_size Maximum number of tasks to keep in the buffer, or 0 for unlimited
     */
    task_queue(int buffer_size) : Done(false), buffer_size(buffer_size), next_task_id(0) {}

    /**
     * @brief Destructor
     */
    ~task_queue() = default;

    /**
     * @brief Marks the queue as done, preventing addition of new tasks
     *
     * This signals to all waiting threads that no more tasks will be added
     * and they should terminate after processing current tasks.
     */
    void mark_done()
    {
        {
            std::lock_guard<std::mutex> lock(this->tasks_mutex);
            this->Done = true;
        }
        this->tasks_cv.notify_all();
    }

    /**
     * @brief Gets the next task ID to be assigned
     *
     * @return The ID that will be assigned to the next task pushed into the queue
     */
    std::uint64_t Getnext_task_id() const { return this->next_task_id; }

    /**
     * @brief Pushes a new task into the queue
     *
     * If the queue is marked as done, no tasks will be added.
     * If a buffer size is specified, older tasks may be removed when the queue exceeds the buffer size.
     *
     * @param task Function object to be executed
     */
    void Push(std::function<R()>&& task)
    {
        if (this->Done)
        {
            return;
        }
        else
        {
            std::lock_guard<std::mutex> lk(this->tasks_mutex);
            // xsigmalog_f(info, "pushing-task %d", (int)this->next_task_id);
            this->Tasks.push(std::make_pair(this->next_task_id++, std::move(task)));
            while (this->buffer_size > 0 &&
                   static_cast<int>(this->Tasks.size()) > this->buffer_size)
            {
                this->Tasks.pop();
            }
        }
        this->tasks_cv.notify_one();
    }

    /**
     * @brief Pops a task from the queue
     *
     * This method will block until either a task is available or the queue is marked as done.
     *
     * @param task_id [out] The ID of the popped task
     * @param task [out] The function object to execute
     * @return true if a task was successfully popped, false if the queue is done and empty
     */
    bool Pop(std::uint64_t& task_id, std::function<R()>& task)
    {
        std::unique_lock<std::mutex> lk(this->tasks_mutex);
        this->tasks_cv.wait(lk, [this] { return this->Done || !this->Tasks.empty(); });
        if (!this->Tasks.empty())
        {
            auto task_pair = this->Tasks.front();
            // xsigmalog_f(trace, "popping-task %d", (int)task_pair.first);
            this->Tasks.pop();
            lk.unlock();

            task_id = task_pair.first;
            task    = std::move(task_pair.second);
            return true;
        }
        assert(this->Done);
        return false;
    }

private:
    std::atomic_bool                                         Done;
    int                                                      buffer_size;
    std::atomic<std::uint64_t>                               next_task_id;
    std::queue<std::pair<std::uint64_t, std::function<R()>>> Tasks;
    std::mutex                                               tasks_mutex;
    std::condition_variable                                  tasks_cv;
};

//=============================================================================
/**
 * @class result_queue
 * @brief Internal result queue implementation that manages function results
 *
 * This class provides a thread-safe priority queue implementation for storing
 * task results along with their task IDs. Results can be popped in task ID order
 * when strict ordering is enabled.
 *
 * @tparam R The type of the results stored in the queue
 */
template <typename R>
class result_queue
{
public:
    /**
     * @brief Constructs a result queue
     *
     * @param strict_ordering If true, results can only be popped in order of task ID
     */
    result_queue(bool strict_ordering) : next_result_id(0), strict_ordering(strict_ordering) {}

    /**
     * @brief Destructor
     */
    ~result_queue() = default;

    /**
     * @brief Gets the next result ID that will be popped
     *
     * @return The ID of the next result expected to be popped
     */
    std::uint64_t Getnext_result_id() const { return this->next_result_id; }

    /**
     * @brief Pushes a new result into the queue
     *
     * Results with IDs less than next_result_id are considered obsolete and discarded.
     *
     * @param task_id The ID of the task that produced this result
     * @param result The result value to store
     */
    void Push(std::uint64_t task_id, const R&& result)
    {
        std::unique_lock<std::mutex> lk(this->results_mutex);
        // don't save this result if it's obsolete.
        if (task_id >= this->next_result_id)
        {
            this->Results.push(std::make_pair(task_id, std::move(result)));
        }
        lk.unlock();
        this->results_cv.notify_one();
    }

    /**
     * @brief Attempts to pop a result without waiting
     *
     * In strict ordering mode, this will only pop if the next expected result is available.
     *
     * @param result [out] The popped result value
     * @return true if a result was successfully popped, false otherwise
     */
    bool try_pop(R& result)
    {
        std::unique_lock<std::mutex> lk(this->results_mutex);
        if (this->Results.empty() ||
            (this->strict_ordering && this->Results.top().first != this->next_result_id))
        {
            // results are not available or of strict-ordering is requested, the
            // result available is not the next one in sequence, hence don't pop
            // anything.
            return false;
        }

        auto result_pair     = this->Results.top();
        this->next_result_id = (result_pair.first + 1);
        this->Results.pop();
        lk.unlock();

        result = std::move(result_pair.second);
        return true;
    }

    /**
     * @brief Pops a result, waiting if necessary
     *
     * This method blocks until a result is available. In strict ordering mode,
     * it waits for the next expected result in sequence.
     *
     * @param result [out] The popped result value
     * @return true if a result was successfully popped, false otherwise
     */
    bool Pop(R& result)
    {
        std::unique_lock<std::mutex> lk(this->results_mutex);
        this->results_cv.wait(
            lk,
            [this]
            {
                return !this->Results.empty() &&
                       (!this->strict_ordering ||
                        this->Results.top().first == this->next_result_id);
            });
        lk.unlock();
        return this->try_pop(result);
    }

private:
    template <typename T>
    struct Comparator
    {
        bool operator()(const T& left, const T& right) const { return left.first > right.first; }
    };
    std::priority_queue<
        std::pair<std::uint64_t, R>,
        std::vector<std::pair<std::uint64_t, R>>,
        Comparator<std::pair<std::uint64_t, R>>>
                               Results;
    std::mutex                 results_mutex;
    std::condition_variable    results_cv;
    std::atomic<std::uint64_t> next_result_id;
    bool                       strict_ordering;
};

//-----------------------------------------------------------------------------
/**
 * @brief Constructor for the threaded task queue
 *
 * Initializes a queue with the specified worker function, thread count, and behavior settings.
 *
 * @param worker Function that will process each task
 * @param strict_ordering If true, results must be popped in the same order tasks were pushed
 * @param buffer_size Maximum task buffer size (-1 for unlimited)
 * @param max_concurrent_tasks Maximum number of worker threads (-1 for system default)
 */
template <typename R, typename... Args>
threaded_task_queue<R, Args...>::threaded_task_queue(
    std::function<R(Args...)> worker,
    bool                      strict_ordering,
    int                       buffer_size,
    int                       max_concurrent_tasks)
    : Worker(worker),
      Tasks(new xsigmathreaded_task_queue_internals::task_queue<R>(
          std::max(0, strict_ordering ? 0 : buffer_size))),
      Results(new xsigmathreaded_task_queue_internals::result_queue<R>(strict_ordering)),
      number_of_threads(
          max_concurrent_tasks <= 0 ? multi_threader::get_global_defaultnumber_of_threads()
                                    : max_concurrent_tasks),
      Threads{new std::thread[this->number_of_threads]}
{
    auto f = [this](int thread_id)
    {
        xsigma::logger::SetThreadName("ttq::worker" + std::to_string(thread_id));
        while (true)
        {
            std::function<R()> task;
            std::uint64_t      task_id;
            if (this->Tasks->Pop(task_id, task))
            {
                this->Results->Push(task_id, task());
                continue;
            }
            else
            {
                break;
            }
        }
        // xsigmalog_f(info, "done");
    };

    for (int cc = 0; cc < this->number_of_threads; ++cc)
    {
        this->Threads[cc] = std::thread(f, cc);
    }
}

//-----------------------------------------------------------------------------
/**
 * @brief Destructor for the threaded task queue
 *
 * Signals all worker threads to terminate and waits for them to join
 * before freeing resources.
 */
template <typename R, typename... Args>
threaded_task_queue<R, Args...>::~threaded_task_queue()
{
    this->Tasks->mark_done();
    for (int cc = 0; cc < this->number_of_threads; ++cc)
    {
        this->Threads[cc].join();
    }
}

//-----------------------------------------------------------------------------
/**
 * @brief Pushes a new task into the queue
 *
 * Binds the provided arguments to the worker function and adds it to the task queue.
 * This method returns immediately without waiting for the task to complete.
 *
 * @param args Arguments to pass to the worker function
 */
template <typename R, typename... Args>
void threaded_task_queue<R, Args...>::Push(Args&&... args)
{
    this->Tasks->Push(std::bind(this->Worker, args...));
}

//-----------------------------------------------------------------------------
/**
 * @brief Attempts to pop a result without waiting
 *
 * @param result [out] The result from a completed task
 * @return true if a result was successfully popped, false if no results are available
 */
template <typename R, typename... Args>
bool threaded_task_queue<R, Args...>::TryPop(R& result)
{
    return this->Results->try_pop(result);
}

//-----------------------------------------------------------------------------
/**
 * @brief Pops a result, returning false if the queue is empty
 *
 * Unlike the internal Results->Pop method, this does not wait if no results
 * are available. It first checks if the queue is empty.
 *
 * @param result [out] The result from a completed task
 * @return true if a result was successfully popped, false if the queue is empty
 */
template <typename R, typename... Args>
bool threaded_task_queue<R, Args...>::Pop(R& result)
{
    if (this->is_empty())
    {
        return false;
    }

    return this->Results->Pop(result);
}

//-----------------------------------------------------------------------------
/**
 * @brief Checks if the task queue is empty
 *
 * A queue is considered empty when all pushed tasks have been completed
 * and their results have been popped.
 *
 * @return true if no tasks are pending or in progress
 */
template <typename R, typename... Args>
bool threaded_task_queue<R, Args...>::IsEmpty() const
{
    return this->Results->Getnext_result_id() == this->Tasks->Getnext_task_id();
}

//-----------------------------------------------------------------------------
/**
 * @brief Flushes the queue by popping all available results
 *
 * This method removes all results from the queue without returning them.
 * It can be used to clear the queue when results are not needed.
 */
template <typename R, typename... Args>
void threaded_task_queue<R, Args...>::Flush()
{
    R tmp;
    while (!this->is_empty())
    {
        this->Pop(tmp);
    }
}

//=============================================================================
// ** specialization for `void` returns types.
//=============================================================================

//-----------------------------------------------------------------------------
/**
 * @brief Constructor for void-returning threaded task queue
 *
 * This specialization handles tasks that don't return results.
 *
 * @param worker Function that will process each task
 * @param strict_ordering If true, tasks must complete in order
 * @param buffer_size Maximum task buffer size (-1 for unlimited)
 * @param max_concurrent_tasks Maximum number of worker threads (-1 for system default)
 */
template <typename... Args>
threaded_task_queue<void, Args...>::threaded_task_queue(
    std::function<void(Args...)> worker,
    bool                         strict_ordering,
    int                          buffer_size,
    int                          max_concurrent_tasks)
    : Worker(worker),
      Tasks(new xsigmathreaded_task_queue_internals::task_queue<void>(
          std::max(0, strict_ordering ? 0 : buffer_size))),
      next_result_id(0),
      number_of_threads(
          max_concurrent_tasks <= 0 ? multi_threader::get_global_defaultnumber_of_threads()
                                    : max_concurrent_tasks),
      Threads{new std::thread[this->number_of_threads]}
{
    auto f = [this](int thread_id)
    {
        xsigma::logger::SetThreadName("ttq::worker" + std::to_string(thread_id));
        while (true)
        {
            std::function<void()> task;
            std::uint64_t         task_id;
            if (this->Tasks->Pop(task_id, task))
            {
                task();

                std::unique_lock<std::mutex> lk(this->next_result_idMutex);
                this->next_result_id =
                    std::max(static_cast<std::uint64_t>(this->next_result_id), task_id + 1);
                lk.unlock();
                this->results_cv.notify_all();
                continue;
            }
            else
            {
                break;
            }
        }
        this->results_cv.notify_all();
        // xsigmalog_f(info, "done");
    };

    for (int cc = 0; cc < this->number_of_threads; ++cc)
    {
        this->Threads[cc] = std::thread(f, cc);
    }
}

//-----------------------------------------------------------------------------
/**
 * @brief Destructor for void-returning threaded task queue
 *
 * Signals all worker threads to terminate and waits for them to join
 * before freeing resources.
 */
template <typename... Args>
threaded_task_queue<void, Args...>::~threaded_task_queue()
{
    this->Tasks->mark_done();
    for (int cc = 0; cc < this->number_of_threads; ++cc)
    {
        this->Threads[cc].join();
    }
}

//-----------------------------------------------------------------------------
/**
 * @brief Pushes a new task into the void-returning queue
 *
 * Binds the provided arguments to the worker function and adds it to the task queue.
 * This method returns immediately without waiting for the task to complete.
 *
 * @param args Arguments to pass to the worker function
 */
template <typename... Args>
void threaded_task_queue<void, Args...>::Push(Args&&... args)
{
    this->Tasks->Push(std::bind(this->Worker, args...));
}

//-----------------------------------------------------------------------------
/**
 * @brief Checks if the void-returning task queue is empty
 *
 * A queue is considered empty when all pushed tasks have been completed.
 *
 * @return true if no tasks are pending or in progress
 */
template <typename... Args>
bool threaded_task_queue<void, Args...>::IsEmpty() const
{
    return this->next_result_id == this->Tasks->Getnext_task_id();
}

//-----------------------------------------------------------------------------
/**
 * @brief Flushes the void-returning queue by waiting for all tasks to complete
 *
 * This method blocks until all tasks have been processed.
 */
template <typename... Args>
void threaded_task_queue<void, Args...>::Flush()
{
    if (this->is_empty())
    {
        return;
    }
    std::unique_lock<std::mutex> lk(this->next_result_idMutex);
    this->results_cv.wait(lk, [this] { return this->is_empty(); });
}

}  // namespace xsigma
