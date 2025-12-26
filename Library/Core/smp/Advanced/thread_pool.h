#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

#include "common/macros.h"
#include "memory/numa.h"
#include "smp/Advanced/thread_name.h"

namespace quarisma::detail::smp::Advanced
{

/**
 * @brief Abstract base class for task-based thread pools.
 *
 * This class defines the interface for thread pool implementations that execute
 * tasks asynchronously on a pool of worker threads. Implementations must provide
 * thread-safe task execution and thread pool management.
 *
 * @note Thread-safe. All methods can be called from multiple threads concurrently.
 */
class QUARISMA_VISIBILITY task_thread_pool_base
{
public:
    /**
     * @brief Queues a task for execution on the thread pool.
     *
     * The task is executed asynchronously on one of the available worker threads.
     * The function returns immediately without waiting for the task to complete.
     *
     * @param func The task to execute. Must be a valid callable.
     *
     * @note Thread-safe.
     */
    virtual void Run(std::function<void()> func) = 0;

    /**
     * @brief Returns the total number of threads in the pool.
     *
     * @return The number of worker threads in this thread pool.
     *
     * @note Thread-safe.
     */
    virtual size_t Size() const = 0;

    /**
     * @brief Returns the number of available (idle) threads in the pool.
     *
     * @return The number of idle threads currently available to execute tasks.
     *
     * @note Thread-safe.
     */
    virtual size_t NumAvailable() const = 0;

    /**
     * @brief Checks if the current thread is a worker thread in this pool.
     *
     * @return true if the current thread is a worker thread in this pool,
     *         false otherwise.
     *
     * @note Thread-safe.
     */
    virtual bool InThreadPool() const = 0;

    virtual ~task_thread_pool_base() noexcept = default;

    /**
     * @brief Returns the default number of threads for a thread pool.
     *
     * The default is based on the system's hardware concurrency. If hardware
     * concurrency cannot be determined, returns 1.
     *
     * @return The recommended number of threads for a thread pool.
     */
    QUARISMA_API static size_t DefaultNumThreads();
};

/**
 * @brief A thread pool implementation for executing tasks asynchronously.
 *
 * thread_pool manages a fixed number of worker threads that execute tasks
 * from a queue. Tasks are executed in FIFO order. The pool supports both
 * tasks without IDs and tasks that receive their thread index as a parameter.
 *
 * Thread Safety:
 * - All public methods are thread-safe and can be called from multiple threads.
 * - The pool uses a mutex and condition variables to synchronize access to
 *   the task queue and thread state.
 * - Worker threads are joined in the destructor, ensuring clean shutdown.
 *
 * Error Handling:
 * - User-provided tasks are responsible for their own error handling.
 * - If a task throws an exception, it is logged and the thread continues
 *   processing the next task.
 *
 * NUMA Support:
 * - If a NUMA node ID is provided, threads are bound to that NUMA node
 *   for better performance on NUMA systems.
 */
class QUARISMA_VISIBILITY thread_pool : public task_thread_pool_base
{
protected:
    struct TaskElement
    {
        bool run_with_id;
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
        const std::function<void()> no_id;
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
        const std::function<void(std::size_t)> with_id;

        explicit TaskElement(std::function<void()> f)
            : run_with_id(false), no_id(std::move(f)), with_id(nullptr)
        {
        }
        explicit TaskElement(std::function<void(std::size_t)> f)
            : run_with_id(true), no_id(nullptr), with_id(std::move(f))
        {
        }
    };

    std::queue<TaskElement>  tasks_;
    std::vector<std::thread> threads_;
    mutable std::mutex       mutex_;
    std::condition_variable  condition_;
    std::condition_variable  completed_;
    std::atomic_bool         running_;
    bool                     complete_;
    std::size_t              available_;
    std::size_t              total_;
    int                      numa_node_id_;

public:
    thread_pool() = delete;

    /**
     * @brief Constructs a thread pool with the specified number of threads.
     *
     * @param pool_size The number of worker threads. If negative, uses the
     *                  default number of threads based on hardware concurrency.
     * @param numa_node_id The NUMA node to bind threads to. -1 means no binding.
     * @param init_thread Optional callback to initialize each worker thread.
     *                    Called once per thread before the main loop starts.
     *
     * @note The pool starts running immediately after construction.
     */
    QUARISMA_API explicit thread_pool(
        int pool_size, int numa_node_id = -1, const std::function<void()>& init_thread = nullptr);

    /**
     * @brief Destructs the thread pool and joins all worker threads.
     *
     * Signals all worker threads to stop and waits for them to exit.
     * Any pending tasks in the queue are discarded.
     */
    QUARISMA_API ~thread_pool() override;

    QUARISMA_API size_t Size() const override;

    QUARISMA_API size_t NumAvailable() const override;

    QUARISMA_API bool InThreadPool() const override;

    QUARISMA_API void Run(std::function<void()> func) override;

    /**
     * @brief Queues a task that receives its thread index as a parameter.
     *
     * The task is executed asynchronously on one of the available worker threads.
     * The thread index (0 to Size()-1) is passed to the task as a parameter.
     *
     * @param task The task to execute. Must accept a std::size_t parameter.
     *
     * @note Thread-safe.
     */
    template <typename Task>
    void RunTaskWithId(Task task)
    {
        std::unique_lock<std::mutex> lock(mutex_);

        // Set task and signal condition variable so that a worker thread will
        // wake up and use the task.
        tasks_.emplace(static_cast<std::function<void(std::size_t)>>(task));
        complete_ = false;
        condition_.notify_one();
    }

    /**
     * @brief Waits for all queued tasks to complete.
     *
     * Blocks the calling thread until the task queue is empty and all worker
     * threads are idle. If new tasks are queued while waiting, this function
     * will wait for those tasks as well.
     *
     * @note Thread-safe.
     */
    QUARISMA_API void WaitWorkComplete();

private:
    /**
     * @brief Entry point for worker threads.
     *
     * Each worker thread runs this function in a loop, waiting for tasks
     * from the queue and executing them. The thread exits when the pool's
     * running flag is set to false.
     *
     * @param index The index of this worker thread (0 to Size()-1).
     */
    void MainLoop(std::size_t index);
};

/**
 * @brief A specialized thread pool for executing tasks with NUMA support.
 *
 * task_thread_pool extends thread_pool with automatic thread naming and NUMA
 * binding. Each worker thread is named "QuarismaTaskThread" and bound to the
 * specified NUMA node for optimal performance on NUMA systems.
 *
 * @note Thread-safe. Inherits all thread-safety guarantees from thread_pool.
 */
class QUARISMA_VISIBILITY task_thread_pool : public thread_pool
{
public:
    /**
     * @brief Constructs a task thread pool with NUMA support.
     *
     * @param pool_size The number of worker threads. If negative, uses the
     *                  default number of threads based on hardware concurrency.
     * @param numa_node_id The NUMA node to bind threads to. -1 means no binding.
     *
     * @note Each worker thread is automatically named and bound to the
     *       specified NUMA node.
     */
    explicit task_thread_pool(int pool_size, int numa_node_id = -1)
        : thread_pool(
              pool_size,
              numa_node_id,
              [numa_node_id]()
              {
                  set_thread_name("QuarismaTaskThread");
                  NUMABind(numa_node_id);
              })
    {
    }
};

/**
 * @brief Factory function to create a task thread pool.
 *
 * Creates and returns a new task_thread_pool instance wrapped in a shared_ptr.
 * This is the recommended way to create thread pools.
 *
 * @param pool_size The number of worker threads. If negative, uses the
 *                  default number of threads based on hardware concurrency.
 * @param numa_node_id The NUMA node to bind threads to. -1 means no binding.
 *
 * @return A shared pointer to the newly created thread pool.
 *
 * @note Thread-safe. The returned pool is ready to use immediately.
 */
QUARISMA_API std::shared_ptr<task_thread_pool_base> create_task_thread_pool(
    int pool_size, int numa_node_id = -1);

}  // namespace quarisma::detail::smp::Advanced
