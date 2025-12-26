#include "smp_new/parallel/parallelize_1d.h"

#include <algorithm>
#include <chrono>
#include <thread>

#include "smp_new/core/thread_pool.h"
#include "smp_new/parallel/parallel_api.h"

namespace xsigma::smp_new::parallel
{

namespace internal
{

// ============================================================================
// WorkStealingDeque Implementation
// ============================================================================

void WorkStealingDeque::push_back(const WorkItem& item)
{
    std::scoped_lock const lock(mutex_);
    items_.push_back(item);
}

bool WorkStealingDeque::pop_back(WorkItem& item)
{
    std::scoped_lock const lock(mutex_);
    if (items_.empty())
    {
        return false;
    }
    item = items_.back();
    items_.pop_back();
    return true;
}

bool WorkStealingDeque::steal_front(WorkItem& item)
{
    std::scoped_lock const lock(mutex_);
    if (items_.empty())
    {
        return false;
    }
    item = items_.front();
    items_.pop_front();
    return true;
}

bool WorkStealingDeque::empty() const
{
    std::scoped_lock const lock(mutex_);
    return items_.empty();
}

size_t WorkStealingDeque::size() const
{
    std::scoped_lock const lock(mutex_);
    return items_.size();
}

void WorkStealingDeque::clear()
{
    std::scoped_lock const lock(mutex_);
    items_.clear();
}

// ============================================================================
// Parallelize1DCoordinator Implementation
// ============================================================================

Parallelize1DCoordinator::Parallelize1DCoordinator(
    const std::function<void(size_t)>& function, size_t range)
    : function_(function), range_(range)
{
}

void Parallelize1DCoordinator::execute()
{
    if (range_ == 0)
    {
        return;
    }

    auto&        pool        = internal::GetIntraopPool();
    size_t const num_threads = std::max<size_t>(1, pool.Size());

    if (deques_.size() != num_threads)
    {
        deques_.clear();
        deques_.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i)
        {
            deques_.push_back(std::make_unique<WorkStealingDeque>());
        }
    }

    for (auto& deque : deques_)
    {
        deque->clear();
    }

    pending_work_.store(0, std::memory_order_relaxed);
    active_workers_.store(num_threads, std::memory_order_relaxed);
    {
        std::scoped_lock const lock(exception_mutex_);
        exception_ = nullptr;
    }

    // Distribute work evenly across threads
    // Each thread gets a chunk of work to start with
    size_t const chunk_size = (range_ + num_threads - 1) / num_threads;

    for (size_t i = 0; i < num_threads; ++i)
    {
        size_t const begin = i * chunk_size;
        if (begin >= range_)
        {
            break;
        }
        size_t const end = std::min(begin + chunk_size, range_);
        enqueue_work(i, {begin, end});
    }

    for (size_t i = 0; i < num_threads; ++i)
    {
        pool.Run([this, i]() { worker_thread_func(*this, i); });
    }

    // Wait for coordinator-tracked work to complete and all workers to finish
    wait_complete();

    // Check for exceptions
    std::exception_ptr captured_exception;
    {
        std::scoped_lock const lock(exception_mutex_);
        captured_exception = exception_;
        exception_         = nullptr;
    }

    if (captured_exception)
    {
        std::rethrow_exception(captured_exception);
    }
}

void Parallelize1DCoordinator::mark_complete()
{
    size_t const previous = pending_work_.fetch_sub(1, std::memory_order_acq_rel);
    if (previous == 1)
    {
        std::scoped_lock const lock(completion_mutex_);
        completion_cv_.notify_all();
    }
}

void Parallelize1DCoordinator::wait_complete()
{
    std::unique_lock<std::mutex> lock(completion_mutex_);
    completion_cv_.wait(
        lock,
        [this]()
        {
            return pending_work_.load(std::memory_order_acquire) == 0 &&
                   active_workers_.load(std::memory_order_acquire) == 0;
        });
}

void Parallelize1DCoordinator::mark_worker_finished()
{
    size_t const previous = active_workers_.fetch_sub(1, std::memory_order_acq_rel);
    if (previous == 1)
    {
        std::scoped_lock const lock(completion_mutex_);
        completion_cv_.notify_all();
    }
}

void Parallelize1DCoordinator::enqueue_work(size_t thread_id, const WorkItem& item)
{
    deques_[thread_id]->push_back(item);
    pending_work_.fetch_add(1, std::memory_order_relaxed);
}

void Parallelize1DCoordinator::set_exception(std::exception_ptr ex)
{
    std::scoped_lock const lock(exception_mutex_);
    if (!exception_)
    {
        exception_ = ex;
    }
}

// ============================================================================
// Worker Thread Function
// ============================================================================

void worker_thread_func(Parallelize1DCoordinator& coordinator, size_t thread_id)
{
    const auto&  function    = coordinator.get_function();
    size_t const num_threads = coordinator.get_num_threads();

    auto process_work_item = [&](internal::WorkItem work)
    {
        try
        {
            for (size_t j = work.begin; j < work.end; ++j)
            {
                function(j);
            }
        }
        catch (...)
        {
            coordinator.set_exception(std::current_exception());
        }
        coordinator.mark_complete();
    };

    try
    {
        WorkItem item;
        while (coordinator.get_deque(thread_id).pop_back(item))
        {
            process_work_item(item);
        }

        // Try to steal work from other threads
        bool found_work = true;
        while (found_work)
        {
            found_work = false;

            // Try to steal from each other thread
            for (size_t i = 0; i < num_threads; ++i)
            {
                if (i == thread_id)
                {
                    continue;
                }

                if (coordinator.get_deque(i).steal_front(item))
                {
                    found_work = true;

                    // Split the stolen work
                    size_t const mid = (item.begin + item.end) / 2;
                    if (mid > item.begin)
                    {
                        // Put second half back for other threads to steal
                        coordinator.enqueue_work(i, {mid, item.end});
                        item.end = mid;
                    }

                    // Process first half
                    process_work_item(item);
                    break;
                }
            }
        }
    }
    catch (...)
    {
        coordinator.set_exception(std::current_exception());
    }

    // Mark this worker thread as finished
    coordinator.mark_worker_finished();
}

}  // namespace internal

// ============================================================================
// Public API
// ============================================================================

void parallelize_1d(const std::function<void(size_t)>& function, size_t range)
{
    if (range == 0)
    {
        return;
    }

    // Create coordinator and execute
    internal::Parallelize1DCoordinator coordinator(function, range);
    coordinator.execute();
}

}  // namespace xsigma::smp_new::parallel
