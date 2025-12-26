#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "common/macros.h"

namespace quarisma::smp_new::core
{

/**
 * @brief Abstract base class for thread pool implementations.
 *
 * Defines the interface for thread pool implementations that execute
 * tasks asynchronously on a pool of worker threads.
 *
 * @note Thread-safe. All methods can be called from multiple threads concurrently.
 */
class QUARISMA_VISIBILITY TaskThreadPoolBase
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

    /**
     * @brief Waits for all queued tasks to complete.
     *
     * Blocks the calling thread until the task queue is empty and all worker
     * threads are idle.
     *
     * @note Thread-safe.
     */
    virtual void WaitWorkComplete() = 0;

    virtual ~TaskThreadPoolBase() noexcept = default;

    /**
     * @brief Returns the default number of threads for a thread pool.
     *
     * The default is based on the system's hardware concurrency.
     *
     * @return The recommended number of threads for a thread pool.
     */
    QUARISMA_API static size_t DefaultNumThreads();
};

/**
 * @brief A thread pool implementation for executing tasks asynchronously.
 *
 * ThreadPool manages a fixed number of worker threads that execute tasks
 * from a queue. Tasks are executed in FIFO order.
 *
 * Thread Safety:
 * - All public methods are thread-safe and can be called from multiple threads.
 * - The pool uses a mutex and condition variables to synchronize access.
 * - Worker threads are joined in the destructor, ensuring clean shutdown.
 *
 * Error Handling:
 * - If a task throws an exception, it is captured and rethrown after all
 *   tasks complete.
 *
 * NUMA Support:
 * - If a NUMA node ID is provided, threads are bound to that NUMA node
 *   for better performance on NUMA systems.
 */
class QUARISMA_VISIBILITY ThreadPool : public TaskThreadPoolBase
{
protected:
    struct TaskElement
    {
        std::function<void()> func;
        explicit TaskElement(std::function<void()> f) : func(std::move(f)) {}
    };

    std::queue<TaskElement>  tasks_;
    std::vector<std::thread> threads_;
    mutable std::mutex       mutex_;
    std::condition_variable  condition_;
    std::condition_variable  completed_;
    std::atomic_bool         running_;
    std::size_t              available_;
    std::size_t              total_;
    std::size_t              pending_tasks_;  // Number of tasks queued or executing
    int                      numa_node_id_;
    std::exception_ptr       exception_;

public:
    ThreadPool() = delete;

    /**
     * @brief Constructs a thread pool with the specified number of threads.
     *
     * @param pool_size The number of worker threads. If negative, uses the
     *                  default number of threads based on hardware concurrency.
     * @param numa_node_id The NUMA node to bind threads to. -1 means no binding.
     *
     * @note The pool starts running immediately after construction.
     */
    QUARISMA_API explicit ThreadPool(int pool_size, int numa_node_id = -1);

    /**
     * @brief Destructs the thread pool and joins all worker threads.
     *
     * Signals all worker threads to stop and waits for them to exit.
     * Any pending tasks in the queue are discarded.
     */
    QUARISMA_API ~ThreadPool() override;

    QUARISMA_API size_t Size() const override;

    QUARISMA_API size_t NumAvailable() const override;

    QUARISMA_API bool InThreadPool() const override;

    QUARISMA_API void Run(std::function<void()> func) override;

    QUARISMA_API void WaitWorkComplete() override;

private:
    /**
     * @brief Entry point for worker threads.
     *
     * Each worker thread runs this function in a loop, waiting for tasks
     * from the queue and executing them.
     *
     * @param index The index of this worker thread (0 to Size()-1).
     */
    void MainLoop(std::size_t index);
};

/**
 * @brief Factory function to create a thread pool.
 *
 * Creates and returns a new ThreadPool instance wrapped in a shared_ptr.
 *
 * @param pool_size The number of worker threads. If negative, uses the
 *                  default number of threads based on hardware concurrency.
 * @param numa_node_id The NUMA node to bind threads to. -1 means no binding.
 *
 * @return A shared pointer to the newly created thread pool.
 *
 * @note Thread-safe. The returned pool is ready to use immediately.
 */
QUARISMA_API std::shared_ptr<TaskThreadPoolBase> CreateThreadPool(
    int pool_size, int numa_node_id = -1);

}  // namespace quarisma::smp_new::core
