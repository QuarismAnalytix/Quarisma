#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "common/macros.h"

namespace quarisma::smp_new::parallel
{

/**
 * @brief Internal implementation details for parallelize_1d.
 *
 * This header contains the internal work-stealing queue and coordinator
 * for the parallelize_1d function. The public API is declared in parallel_api.h.
 *
 * @internal
 */
namespace internal
{

/**
 * @brief Work item in the work-stealing queue.
 */
struct WorkItem
{
    size_t begin;  ///< Start index (inclusive)
    size_t end;    ///< End index (exclusive)
};

/**
 * @brief Work-stealing deque for load balancing.
 *
 * Each worker thread has its own deque. When a thread runs out of work,
 * it steals from other threads' dequeues.
 */
class WorkStealingDeque
{
public:
    /**
     * @brief Default constructor.
     */
    WorkStealingDeque() = default;

    /**
     * @brief Deleted copy constructor (non-copyable due to mutex).
     */
    WorkStealingDeque(const WorkStealingDeque&) = delete;

    /**
     * @brief Deleted copy assignment (non-copyable due to mutex).
     */
    WorkStealingDeque& operator=(const WorkStealingDeque&) = delete;

    /**
     * @brief Deleted move constructor (non-movable due to mutex).
     */
    WorkStealingDeque(WorkStealingDeque&&) = delete;

    /**
     * @brief Deleted move assignment (non-movable due to mutex).
     */
    WorkStealingDeque& operator=(WorkStealingDeque&&) = delete;

    /**
     * @brief Push work item to the back (owner thread only).
     */
    void push_back(const WorkItem& item);

    /**
     * @brief Pop work item from the back (owner thread only).
     * @return true if item was popped, false if deque is empty
     */
    bool pop_back(WorkItem& item);

    /**
     * @brief Steal work item from the front (other threads).
     * @return true if item was stolen, false if deque is empty
     */
    bool steal_front(WorkItem& item);

    /**
     * @brief Check if deque is empty.
     */
    bool empty() const;

    /**
     * @brief Get number of items in deque.
     */
    size_t size() const;

    /**
     * @brief Remove all items from deque.
     */
    void clear();

private:
    mutable std::mutex   mutex_;
    std::deque<WorkItem> items_;
};

/**
 * @brief Coordinator for parallelize_1d execution.
 *
 * Manages work distribution, load balancing, and synchronization.
 */
class Parallelize1DCoordinator
{
public:
    /**
     * @brief Initialize coordinator with function and range.
     */
    Parallelize1DCoordinator(const std::function<void(size_t)>& function, size_t range);

    /**
     * @brief Execute the parallel work.
     */
    void execute();

    /**
     * @brief Get the function to execute.
     */
    const std::function<void(size_t)>& get_function() const { return function_; }

    /**
     * @brief Get the total range.
     */
    size_t get_range() const { return range_; }

    /**
     * @brief Get work-stealing deque for a thread.
     */
    WorkStealingDeque& get_deque(size_t thread_id) { return *deques_[thread_id]; }

    /**
     * @brief Enqueue work for a specific thread and update bookkeeping.
     */
    void enqueue_work(size_t thread_id, const WorkItem& item);

    /**
     * @brief Get number of threads.
     */
    size_t get_num_threads() const { return deques_.size(); }

    /**
     * @brief Mark work as complete and signal waiting threads.
     */
    void mark_complete();

    /**
     * @brief Wait for all work to complete.
     */
    void wait_complete();

    /**
     * @brief Check if exception occurred.
     */
    std::exception_ptr get_exception() const { return exception_; }

    /**
     * @brief Set exception if one occurred.
     */
    void set_exception(std::exception_ptr ex);

    /**
     * @brief Mark a worker thread as finished.
     *
     * Called by worker threads when they complete execution.
     * Signals the completion condition variable if all workers are done.
     */
    void mark_worker_finished();

private:
    const std::function<void(size_t)>&              function_;
    size_t                                          range_;
    std::vector<std::unique_ptr<WorkStealingDeque>> deques_;
    std::atomic<size_t>                             pending_work_{0};
    std::atomic<size_t>                             active_workers_{0};
    std::mutex                                      completion_mutex_;
    std::condition_variable                         completion_cv_;
    std::exception_ptr                              exception_;
    std::mutex                                      exception_mutex_;
};

/**
 * @brief Worker thread function for parallelize_1d.
 *
 * Each worker thread runs this function, processing work from its deque
 * and stealing from other threads' dequeues when idle.
 */
void worker_thread_func(Parallelize1DCoordinator& coordinator, size_t thread_id);

}  // namespace internal

/**
 * @brief Optimized 1D data-parallel work distribution with work-stealing.
 *
 * This function provides high-performance data-parallel execution equivalent to
 * PyTorch's pthreadpool_parallelize_1d(). It distributes a 1D range across
 * multiple threads with minimal overhead using work-stealing for load balancing.
 *
 * Key Features:
 * - Range-based work distribution (not task-based)
 * - Work-stealing deque for load balancing
 * - Minimal overhead per work item (~0.1-0.2 μs)
 * - Blocking execution (waits for all work to complete)
 * - Exception handling and propagation
 *
 * Performance:
 * - Target: Within 2x of PyTorch's pthreadpool_parallelize_1d
 * - Expected improvement: 10-100x faster than task-based parallel_for
 *
 * @param function Function to execute for each item in the range.
 *                 Signature: void(size_t item_index)
 * @param range    Number of items to process (0 to range-1)
 *
 * @note Thread-safe. Can be called from multiple threads (calls are serialized).
 * @note Blocking function - returns only after all work is complete.
 * @note Exceptions thrown in worker threads are captured and rethrown.
 *
 * Example:
 * @code
 * std::vector<float> data(1000000);
 * quarisma::smp_new::parallel::parallelize_1d(
 *     [&data](size_t i) {
 *         data[i] = std::sin(i * 0.001f);
 *     },
 *     data.size()
 * );
 * @endcode
 */
QUARISMA_API void parallelize_1d(const std::function<void(size_t)>& function, size_t range);

}  // namespace quarisma::smp_new::parallel
