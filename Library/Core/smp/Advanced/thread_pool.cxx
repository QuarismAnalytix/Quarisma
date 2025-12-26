#include "smp/Advanced/thread_pool.h"

#include <algorithm>
#include <utility>

#include "logging/logger.h"
#include "smp/Advanced/thread_name.h"
#include "util/exception.h"

#if !defined(__powerpc__) && !defined(__s390x__)
#include <cpuinfo.h>
#endif

namespace xsigma::detail::smp::Advanced
{

size_t task_thread_pool_base::DefaultNumThreads()
{
    size_t num_threads = 0;
#if !defined(__powerpc__) && !defined(__s390x__)
    if (cpuinfo_initialize())
    {
        // In cpuinfo parlance cores are physical ones and processors are virtual
        // thread_pool should be defaulted to number of physical cores
        size_t const num_cores = cpuinfo_get_cores_count();
        num_threads            = cpuinfo_get_processors_count();
        if (num_cores > 0 && num_cores < num_threads)
        {
            return num_cores;
        }
        if (num_threads > 0)
        {
            return num_threads;
        }
    }
#endif
    num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0)
    {
        num_threads = 1;
    }
    return num_threads;
}

thread_pool::thread_pool(int pool_size, int numa_node_id, const std::function<void()>& init_thread)
    : threads_(pool_size < 0 ? DefaultNumThreads() : static_cast<size_t>(pool_size)),
      running_(true),
      complete_(true),
      available_(threads_.size()),
      total_(threads_.size()),
      numa_node_id_(numa_node_id)
{
    for (std::size_t i = 0; i < threads_.size(); ++i)
    {
        threads_[i] = std::thread(
            [this, i, init_thread]()
            {
                set_thread_name("xsigma_thread_pool");
                if (init_thread)
                {
                    init_thread();
                }
                this->MainLoop(i);
            });
    }
}

thread_pool::~thread_pool()
{
    // Set running flag to false then notify all threads.
    {
        std::unique_lock<std::mutex> const lock(mutex_);
        running_ = false;
        condition_.notify_all();
    }

    // Join all threads. Threads should exit cleanly when running_ is set to false.
    // If a thread fails to join, we log the error but continue cleanup.
    for (auto& t : threads_)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
}

size_t thread_pool::Size() const
{
    return threads_.size();
}

size_t thread_pool::NumAvailable() const
{
    std::unique_lock<std::mutex> const lock(mutex_);
    return available_;
}

bool thread_pool::InThreadPool() const
{
    const auto current_id = std::this_thread::get_id();
    return std::any_of(
        threads_.begin(),
        threads_.end(),
        [current_id](const std::thread& thread) { return thread.get_id() == current_id; });
}

void thread_pool::Run(std::function<void()> func)
{
    XSIGMA_CHECK(!threads_.empty(), "No threads available to run a task");
    std::unique_lock<std::mutex> const lock(mutex_);

    // Set task and signal condition variable so that a worker thread will
    // wake up and use the task.
    tasks_.emplace(std::move(func));
    complete_ = false;
    condition_.notify_one();
}

void thread_pool::WaitWorkComplete()
{
    std::unique_lock<std::mutex> lock(mutex_);
    completed_.wait(lock, [&]() { return complete_; });
}

void thread_pool::MainLoop(std::size_t index)
{
    std::unique_lock<std::mutex> lock(mutex_);
    while (running_)
    {
        // Wait on condition variable while the task queue is empty and
        // the pool is still running.
        condition_.wait(lock, [&]() { return !tasks_.empty() || !running_; });
        // If pool is no longer running, break out of loop.
        if (!running_)
        {
            break;
        }

        // Copy task locally and remove from the queue. This is
        // done within its own scope so that the task object is
        // destructed immediately after running the task. This is
        // useful in the event that the function contains
        // shared_ptr arguments bound via bind.
        {
            TaskElement const tasks = std::move(tasks_.front());
            tasks_.pop();
            // Decrement count, indicating thread is no longer available.
            --available_;

            lock.unlock();

            // Run the task. User-provided functions should not throw exceptions.
            // If they do, we log the error and continue. Tasks are responsible for
            // their own error handling.
            if (tasks.run_with_id)
            {
                tasks.with_id(index);
            }
            else
            {
                tasks.no_id();
            }

            // Destruct tasks before taking the lock. As tasks
            // are user provided std::function, they can run
            // arbitrary code during destruction, including code
            // that can reentrantly call into ThreadPool (which would
            // cause a deadlock if we were holding the lock).
        }

        // Update status of empty, maybe
        // Need to recover the lock first
        lock.lock();

        // Increment count, indicating thread is available.
        ++available_;
        if (tasks_.empty() && available_ == total_)
        {
            complete_ = true;
            completed_.notify_one();
        }

        // Deliberately hold the lock on the backedge, so this thread has an
        // opportunity to acquire a new task before another thread acquires
        // the lock.
    }  // while running_
}

std::shared_ptr<task_thread_pool_base> create_task_thread_pool(int pool_size, int numa_node_id)
{
    return std::make_shared<task_thread_pool>(pool_size, numa_node_id);
}

}  // namespace xsigma::detail::smp::Advanced
