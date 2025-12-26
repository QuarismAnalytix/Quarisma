#include "smp_new/core/thread_pool.h"

#include <algorithm>
#include <iostream>
#include <thread>

#include "memory/numa.h"
#include "smp/Advanced/thread_name.h"

namespace xsigma::smp_new::core
{

size_t TaskThreadPoolBase::DefaultNumThreads()
{
    auto num_threads = std::thread::hardware_concurrency();
    return num_threads > 0 ? num_threads : 1;
}

ThreadPool::ThreadPool(int pool_size, int numa_node_id)
    : running_(true),
      available_(0),
      total_(0),
      pending_tasks_(0),
      numa_node_id_(numa_node_id),
      exception_(nullptr)
{
    if (pool_size < 0)
    {
        pool_size = static_cast<int>(DefaultNumThreads());
    }

    total_     = pool_size;
    available_ = pool_size;

    // Create worker threads
    for (int i = 0; i < pool_size; ++i)
    {
        threads_.emplace_back([this, i]() { MainLoop(i); });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> const lock(mutex_);
        running_ = false;
    }
    condition_.notify_all();

    for (auto& thread : threads_)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }
}

size_t ThreadPool::Size() const
{
    std::scoped_lock const lock(mutex_);
    return total_;
}

size_t ThreadPool::NumAvailable() const
{
    std::scoped_lock const lock(mutex_);
    return available_;
}

bool ThreadPool::InThreadPool() const
{
    std::scoped_lock const lock(mutex_);
    const auto             current_id = std::this_thread::get_id();
    return std::any_of(
        threads_.begin(),
        threads_.end(),
        [current_id](const std::thread& thread) { return thread.get_id() == current_id; });
}

void ThreadPool::Run(std::function<void()> func)
{
    std::unique_lock<std::mutex> lock(mutex_);
    tasks_.emplace(std::move(func));
    pending_tasks_++;
    lock.unlock();
    condition_.notify_one();
}

void ThreadPool::WaitWorkComplete()
{
    std::unique_lock<std::mutex> lock(mutex_);
    completed_.wait(lock, [this] { return pending_tasks_ == 0; });

    if (exception_)
    {
        auto ex    = exception_;
        exception_ = nullptr;
        std::rethrow_exception(ex);
    }
}

void ThreadPool::MainLoop(std::size_t index)
{
    // Set thread name for debugging
    xsigma::detail::smp::Advanced::set_thread_name("XSigmaWorker-" + std::to_string(index));

    // Bind to NUMA node if specified
    if (numa_node_id_ >= 0)
    {
        xsigma::NUMABind(numa_node_id_);
    }

    while (running_)
    {
        std::unique_lock<std::mutex> lock(mutex_);

        // Wait for work
        condition_.wait(lock, [this] { return !tasks_.empty() || !running_; });

        if (!running_)
        {
            break;
        }

        if (!tasks_.empty())
        {
            TaskElement const task = std::move(tasks_.front());
            tasks_.pop();
            available_--;

            lock.unlock();

            // Execute task with exception handling
            try
            {
                task.func();
            }
            catch (...)
            {
                // Protect exception_ write with lock to prevent data race
                lock.lock();
                if (!exception_)
                {
                    exception_ = std::current_exception();
                }
                lock.unlock();
            }

            lock.lock();
            available_++;
            pending_tasks_--;

            // Notify if all tasks are complete
            if (pending_tasks_ == 0)
            {
                completed_.notify_one();
            }
        }
    }
}

std::shared_ptr<TaskThreadPoolBase> CreateThreadPool(int pool_size, int numa_node_id)
{
    return std::make_shared<ThreadPool>(pool_size, numa_node_id);
}

}  // namespace xsigma::smp_new::core
