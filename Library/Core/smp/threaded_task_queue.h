/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * This file is part of XSigma and is licensed under a dual-license model:
 *
 *   - Open-source License (GPLv3):
 *       Free for personal, academic, and research use under the terms of
 *       the GNU General Public License v3.0 or later.
 *
 *   - Commercial License:
 *       A commercial license is required for proprietary, closed-source,
 *       or SaaS usage. Contact us to obtain a commercial agreement.
 *
 * Contact: licensing@xsigma.co.uk
 * Website: https://www.xsigma.co.uk
 *
 * Portions of this code are based on VTK (Visualization Toolkit):

 *   Licensed under BSD-3-Clause
 */

/**
 * @class threaded_task_queue
 * @brief simple threaded task queue
 *
 * threaded_task_queue provides a simple task queue that can use threads to
 * execute individual tasks. It is intended for use applications such as data
 * compression, encoding etc. where the task may be completed concurrently
 * without blocking the main thread.
 *
 * threaded_task_queue's API is intended to be called from the same main thread.
 * The constructor defines the work (or task) to be performed. `push` allows the
 * caller to enqueue a task with specified input arguments. The call will return
 * immediately without blocking. The task is enqueued and will be executed
 * concurrently when resources become available. `pop` will block until the
 * result is available. To avoid waiting for results to be available, use
 * `try_pop`.
 *
 * The constructor allows mechanism to customize the queue. `strict_ordering`
 * implies that results should be popped in the same order that tasks were
 * pushed without dropping any task. If the caller is only concerned with
 * obtaining the latest available result where intermediate results that take
 * longer to compute may be dropped, then `strict_ordering` can be set to `false`.
 *
 * `max_concurrent_tasks` controls how many threads are used to process tasks in
 * the queue. Default is same as `multi_threader::get_global_default_number_of_threads()`.
 *
 * `buffer_size` indicates how many tasks may be queued for processing. Default
 * is infinite size. If a positive number is provided, then pushing additional
 * tasks will result in discarding of older tasks that haven't begun processing
 * from the queue. Note, this does not impact tasks that may already be in
 * progress. Also, if `strict_ordering` is true, this is ignored; the
 * buffer_size will be set to unlimited.
 */

#ifndef THREADED_TASK_QUEUE_H
#define THREADED_TASK_QUEUE_H

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "common/export.h"
#include "multi_threader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace threaded_task_queue_internals
{

template <typename R>
class task_queue;

template <typename R>
class result_queue;

}  // namespace threaded_task_queue_internals
#endif  // DOXYGEN_SHOULD_SKIP_THIS

template <typename R, typename... Args>
class XSIGMA_VISIBILITY threaded_task_queue
{
public:
    XSIGMA_API threaded_task_queue(
        std::function<R(Args...)> worker,
        bool                      strict_ordering      = true,
        int                       buffer_size          = -1,
        int                       max_concurrent_tasks = -1);
    XSIGMA_API ~threaded_task_queue();

    /**
   * Push arguments for the work
   */
    XSIGMA_API void push(Args&&... args);

    /**
   * Pop the last result. Returns true on success. May fail if called on an
   * empty queue. This will wait for result to be available.
   */
    XSIGMA_API bool pop(R& result);

    /**
   * Attempt to pop without waiting. If no results are available, returns
   * false.
   */
    XSIGMA_API bool try_pop(R& result);

    /**
   * Returns false if there's some result that may be popped right now or in the
   * future.
   */
    XSIGMA_API bool is_empty() const;

    /**
   * Blocks till the queue becomes empty.
   */
    XSIGMA_API void flush();

private:
    threaded_task_queue(const threaded_task_queue&) = delete;
    void operator=(const threaded_task_queue&)      = delete;

    std::function<R(Args...)>                                       m_worker;
    std::unique_ptr<threaded_task_queue_internals::task_queue<R>>   m_tasks;
    std::unique_ptr<threaded_task_queue_internals::result_queue<R>> m_results;
    int                                                             m_number_of_threads;
    std::unique_ptr<std::thread[]>                                  threads_;
};

template <typename... Args>
class XSIGMA_VISIBILITY threaded_task_queue<void, Args...>
{
public:
    XSIGMA_API threaded_task_queue(
        std::function<void(Args...)> worker,
        bool                         strict_ordering      = true,
        int                          buffer_size          = -1,
        int                          max_concurrent_tasks = -1);
    XSIGMA_API ~threaded_task_queue();

    /**
   * Push arguments for the work
   */
    XSIGMA_API void push(Args&&... args);

    /**
   * Returns false if there's some result that may be popped right now or in the
   * future.
   */
    XSIGMA_API bool is_empty() const;

    /**
   * Blocks till the queue becomes empty.
   */
    XSIGMA_API void flush();

private:
    threaded_task_queue(const threaded_task_queue&) = delete;
    void operator=(const threaded_task_queue&)      = delete;

    std::function<void(Args...)>                                     m_worker;
    std::unique_ptr<threaded_task_queue_internals::task_queue<void>> m_tasks;
    std::condition_variable                                          m_results_cv;
    std::mutex                                                       m_next_result_id_mutex;
    std::atomic<std::uint64_t>                                       m_next_result_id;
    int                                                              m_number_of_threads;
    std::unique_ptr<std::thread[]>                                   threads_;
};

// ========== Template Implementation ==========

#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace threaded_task_queue_internals
{

template <typename R>
class task_queue
{
public:
    task_queue(int buffer_size) : m_done(false), m_buffer_size(buffer_size), m_next_task_id(0) {}

    ~task_queue() = default;

    void mark_done()
    {
        {
            std::lock_guard<std::mutex> lock(m_tasks_mutex);
            m_done = true;
        }
        m_tasks_cv.notify_all();
    }

    std::uint64_t get_next_task_id() const { return m_next_task_id; }

    void push(std::function<R()>&& task)
    {
        if (m_done)
        {
            return;
        }
        else
        {
            std::lock_guard<std::mutex> lk(m_tasks_mutex);
            m_tasks.push(std::make_pair(m_next_task_id++, std::move(task)));
            while (m_buffer_size > 0 && static_cast<int>(m_tasks.size()) > m_buffer_size)
            {
                m_tasks.pop();
            }
        }
        m_tasks_cv.notify_one();
    }

    bool pop(std::uint64_t& task_id, std::function<R()>& task)
    {
        std::unique_lock<std::mutex> lk(m_tasks_mutex);
        m_tasks_cv.wait(lk, [this] { return m_done || !m_tasks.empty(); });
        if (!m_tasks.empty())
        {
            auto task_pair = m_tasks.front();
            m_tasks.pop();
            lk.unlock();

            task_id = task_pair.first;
            task    = std::move(task_pair.second);
            return true;
        }
        assert(m_done);
        return false;
    }

private:
    std::atomic_bool                                         m_done;
    int                                                      m_buffer_size;
    std::atomic<std::uint64_t>                               m_next_task_id;
    std::queue<std::pair<std::uint64_t, std::function<R()>>> m_tasks;
    std::mutex                                               m_tasks_mutex;
    std::condition_variable                                  m_tasks_cv;
};

//=============================================================================
template <typename R>
class result_queue
{
public:
    result_queue(bool strict_ordering) : m_next_result_id(0), m_strict_ordering(strict_ordering) {}

    ~result_queue() = default;

    std::uint64_t get_next_result_id() const { return m_next_result_id; }

    void push(std::uint64_t task_id, const R&& result)
    {
        std::unique_lock<std::mutex> lk(m_results_mutex);
        if (task_id >= m_next_result_id)
        {
            m_results.push(std::make_pair(task_id, std::move(result)));
        }
        lk.unlock();
        m_results_cv.notify_one();
    }

    bool try_pop(R& result)
    {
        std::unique_lock<std::mutex> lk(m_results_mutex);
        if (m_results.empty() || (m_strict_ordering && m_results.top().first != m_next_result_id))
        {
            return false;
        }

        auto result_pair = m_results.top();
        m_next_result_id = (result_pair.first + 1);
        m_results.pop();
        lk.unlock();

        result = std::move(result_pair.second);
        return true;
    }

    bool pop(R& result)
    {
        std::unique_lock<std::mutex> lk(m_results_mutex);
        m_results_cv.wait(
            lk,
            [this]
            {
                return !m_results.empty() &&
                       (!m_strict_ordering || m_results.top().first == m_next_result_id);
            });
        lk.unlock();
        return this->try_pop(result);
    }

private:
    template <typename T>
    struct comparator
    {
        bool operator()(const T& left, const T& right) const { return left.first > right.first; }
    };
    std::priority_queue<
        std::pair<std::uint64_t, R>,
        std::vector<std::pair<std::uint64_t, R>>,
        comparator<std::pair<std::uint64_t, R>>>
                               m_results;
    std::mutex                 m_results_mutex;
    std::condition_variable    m_results_cv;
    std::atomic<std::uint64_t> m_next_result_id;
    bool                       m_strict_ordering;
};

}  // namespace threaded_task_queue_internals
#endif  // DOXYGEN_SHOULD_SKIP_THIS

//-----------------------------------------------------------------------------
template <typename R, typename... Args>
threaded_task_queue<R, Args...>::threaded_task_queue(
    std::function<R(Args...)> worker,
    bool                      strict_ordering,
    int                       buffer_size,
    int                       max_concurrent_tasks)
    : m_worker(worker),
      m_tasks(new threaded_task_queue_internals::task_queue<R>(
          std::max(0, strict_ordering ? 0 : buffer_size))),
      m_results(new threaded_task_queue_internals::result_queue<R>(strict_ordering)),
      m_number_of_threads(
          max_concurrent_tasks <= 0 ? multi_threader::get_global_default_number_of_threads()
                                    : max_concurrent_tasks),
      threads_{new std::thread[m_number_of_threads]}
{
    auto f = [this](int thread_id)
    {
        while (true)
        {
            std::function<R()> task;
            std::uint64_t      task_id;
            if (m_tasks->pop(task_id, task))
            {
                m_results->push(task_id, task());
                continue;
            }
            else
            {
                break;
            }
        }
    };

    for (int cc = 0; cc < m_number_of_threads; ++cc)
    {
        threads_[cc] = std::thread(f, cc);
    }
}

//-----------------------------------------------------------------------------
template <typename R, typename... Args>
threaded_task_queue<R, Args...>::~threaded_task_queue()
{
    m_tasks->mark_done();
    for (int cc = 0; cc < m_number_of_threads; ++cc)
    {
        threads_[cc].join();
    }
}

//-----------------------------------------------------------------------------
template <typename R, typename... Args>
void threaded_task_queue<R, Args...>::push(Args&&... args)
{
    m_tasks->push([this, arguments = std::make_tuple(std::forward<Args>(args)...)]()
                  { return std::apply(m_worker, arguments); });
}

//-----------------------------------------------------------------------------
template <typename R, typename... Args>
bool threaded_task_queue<R, Args...>::try_pop(R& result)
{
    return m_results->try_pop(result);
}

//-----------------------------------------------------------------------------
template <typename R, typename... Args>
bool threaded_task_queue<R, Args...>::pop(R& result)
{
    if (this->is_empty())
    {
        return false;
    }

    return m_results->pop(result);
}

//-----------------------------------------------------------------------------
template <typename R, typename... Args>
bool threaded_task_queue<R, Args...>::is_empty() const
{
    return m_results->get_next_result_id() == m_tasks->get_next_task_id();
}

//-----------------------------------------------------------------------------
template <typename R, typename... Args>
void threaded_task_queue<R, Args...>::flush()
{
    R tmp;
    while (!this->is_empty())
    {
        this->pop(tmp);
    }
}

//=============================================================================
// ** specialization for `void` return types.
//=============================================================================

//-----------------------------------------------------------------------------
template <typename... Args>
threaded_task_queue<void, Args...>::threaded_task_queue(
    std::function<void(Args...)> worker,
    bool                         strict_ordering,
    int                          buffer_size,
    int                          max_concurrent_tasks)
    : m_worker(worker),
      m_tasks(new threaded_task_queue_internals::task_queue<void>(
          std::max(0, strict_ordering ? 0 : buffer_size))),
      m_next_result_id(0),
      m_number_of_threads(
          max_concurrent_tasks <= 0 ? multi_threader::get_global_default_number_of_threads()
                                    : max_concurrent_tasks),
      threads_{new std::thread[m_number_of_threads]}
{
    auto f = [this](int thread_id)
    {
        while (true)
        {
            std::function<void()> task;
            std::uint64_t         task_id;
            if (m_tasks->pop(task_id, task))
            {
                task();

                std::unique_lock<std::mutex> lk(m_next_result_id_mutex);
                m_next_result_id =
                    std::max(static_cast<std::uint64_t>(m_next_result_id), task_id + 1);
                lk.unlock();
                m_results_cv.notify_all();
                continue;
            }
            else
            {
                break;
            }
        }
        m_results_cv.notify_all();
    };

    for (int cc = 0; cc < m_number_of_threads; ++cc)
    {
        threads_[cc] = std::thread(f, cc);
    }
}

//-----------------------------------------------------------------------------
template <typename... Args>
threaded_task_queue<void, Args...>::~threaded_task_queue()
{
    m_tasks->mark_done();
    for (int cc = 0; cc < m_number_of_threads; ++cc)
    {
        threads_[cc].join();
    }
}

//-----------------------------------------------------------------------------
template <typename... Args>
void threaded_task_queue<void, Args...>::push(Args&&... args)
{
    m_tasks->push([this, arguments = std::make_tuple(std::forward<Args>(args)...)]()
                  { std::apply(m_worker, arguments); });
}

//-----------------------------------------------------------------------------
template <typename... Args>
bool threaded_task_queue<void, Args...>::is_empty() const
{
    return m_next_result_id == m_tasks->get_next_task_id();
}

//-----------------------------------------------------------------------------
template <typename... Args>
void threaded_task_queue<void, Args...>::flush()
{
    if (this->is_empty())
    {
        return;
    }
    std::unique_lock<std::mutex> lk(m_next_result_id_mutex);
    m_results_cv.wait(lk, [this] { return this->is_empty(); });
}

#endif
