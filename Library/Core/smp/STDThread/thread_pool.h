
// This file incorporates code from the Visualization Toolkit (VTK) and remains subject to the BSD-3-Clause VTK license.

/**
 * @class thread_pool
 * @brief A thread pool implementation using std::thread
 *
 * thread_pool class creates a thread pool of std::thread, the number
 * of thread must be specified at the initialization of the class.
 * The DoJob() method is used attributes the job to a free thread, if all
 * threads are working, the job is kept in a queue. Note that thread_pool
 * destructor joins threads and finish the jobs in the queue.
 */

#ifndef thread_pool_h
#define thread_pool_h

#include <atomic>              // For std::atomic
#include <condition_variable>  // For std::condition_variable
#include <functional>          // For std::function
#include <mutex>               // For std::unique_lock
#include <queue>               // For std::queue
#include <thread>              // For std::thread
#include <vector>              // For std::vector

#include "common/macros.h"

namespace quarisma
{
namespace detail
{
namespace smp
{

/**
 * @brief Internal thread pool implementation used in smp functions
 *
 * This class is designed to be a Singleton thread pool, but local pool can be allocated too.
 * This thread pool use a Proxy system that is used to allocate a certain amount of threads from
 * the pool, which enable support for smp local scopes.
 * You need to have a Proxy to submit job to the pool.
 */
class QUARISMA_VISIBILITY thread_pool
{
    // Internal data structures
    struct ThreadJob;
    struct ThreadData;
    struct ProxyThreadData;
    struct ProxyData;

public:
    /**
   * @brief Proxy class used to submit work to the thread pool.
   *
   * A proxy act like a single thread pool, but it submits work to its parent thread pool.
   * Using a proxy from multiple threads at the same time is undefined behaviour.
   *
   * Note: Even if nothing prevent a proxy to be moved around threads, it should either be used in
   * the creating thread or in a thread that does not belong to the pool, otherwise it may create a
   * deadlock when joining.
   */
    class QUARISMA_VISIBILITY Proxy final
    {
    public:
        /**
     * @brief Destructor
     *
     * Join must have been called since the last DoJob before destroying the proxy.
     */
        QUARISMA_API ~Proxy();
        Proxy(const Proxy&)                       = delete;
        Proxy&            operator=(const Proxy&) = delete;
        QUARISMA_API        Proxy(Proxy&&) noexcept;
        QUARISMA_API Proxy& operator=(Proxy&&) noexcept;

        /**
     * @brief Blocks calling thread until all jobs are done.
     *
     * Note: nested proxies may execute jobs on calling thread during this function to maximize
     * parallelism.
     */
        QUARISMA_API void Join();

        /**
     * @brief Add a job to the thread pool queue
     *
     * @param job Function object to be executed by a thread in the pool
     */
        QUARISMA_API void DoJob(std::function<void()> job);

        /**
     * @brief Get a reference on all system threads used by this proxy
     *
     * @return Vector of references to the std::thread objects used by this proxy
     */
        std::vector<std::reference_wrapper<std::thread>> GetThreads() const;

        /**
     * @brief Return true is this proxy is allocated from a thread that does not belong to the pool
     *
     * @return true if this proxy is a top-level proxy, false if nested
     */
        bool IsTopLevel() const noexcept;

    private:
        friend class thread_pool;  // Only the thread pool can construct this object

        /**
         * @brief Internal constructor used by thread pool
         *
         * @param data Proxy data containing the proxy's state and worker threads
         */
        Proxy(std::unique_ptr<ProxyData>&& data);

        std::unique_ptr<ProxyData> Data;
    };

    /**
     * @brief Constructor
     *
     * Creates a thread pool with the default number of threads based on hardware concurrency.
     * Initializes internal data structures and starts worker threads.
     */
    thread_pool();

    /**
     * @brief Destructor
     *
     * Joins all worker threads and ensures all pending jobs are completed
     * before destroying the thread pool.
     */
    ~thread_pool();

    thread_pool(const thread_pool&)            = delete;
    thread_pool& operator=(const thread_pool&) = delete;

    /**
   * @brief Create a proxy
   *
   * Create a proxy that will use at most threadCount thread of the thread pool.
   * Proxy act as a thread pool on its own, but will in practice submit its work to this pool,
   * this prevent threads to be created every time a smp function is called.
   *
   * If the current thread not in the pool, it will create a "top-level" proxy, otherwise it will
   * create a nested proxy. A nested proxy will never use a thread that is already in use by its
   * "parent" proxies to prevent deadlocks. It means that nested paralism may have a more limited
   * amount of threads.
   *
   * @param threadCount max amount of thread to use. If 0, uses the number of thread of the pool.
   * If greater than the number of thread of the pool, uses the number of thread of the pool.
   * @return A proxy.
   */
    QUARISMA_API Proxy AllocateThreads(std::size_t threadCount = 0);

    /**
   * Value returned by `GetThreadID` when called by a thread that does not belong to the pool.
   */
    static constexpr std::size_t ExternalThreadID = 1;

    /**
   * @brief Get caller proxy thread virtual ID
   *
   * This function must be called from a proxy thread.
   * If this function is called from non proxy thread, returns `ExternalThreadID`.
   * Valid proxy thread virtual ID are always >= 2
   *
   * @return The thread ID of the current thread within the pool, or ExternalThreadID if called from outside
   */
    QUARISMA_API std::size_t GetThreadId() const;

    /**
   * @brief Returns true when called from a proxy thread, false otherwise.
   *
   * @return true if the current thread is a pool thread within a proxy scope
   */
    QUARISMA_API bool IsParallelScope() const noexcept;

    /**
   * @brief Returns true for a single proxy thread, false for the others.
   *
   * This is used to implement single-threaded behavior in certain contexts.
   *
   * @return true if the current thread is designated as the "single thread" in the proxy
   */
    QUARISMA_API bool GetSingleThread() const;

    /**
   * @brief Returns number of system thread used by the thread pool.
   *
   * @return The total number of threads managed by this pool
   */
    QUARISMA_API std::size_t ThreadCount() const noexcept;

private:
    /**
     * @brief Run a job from the job queue
     *
     * @param data Thread data containing the job queue
     * @param jobIndex Index of the job to run
     * @param lock Mutex lock that must be unlocked during execution and relocked after
     */
    static void RunJob(ThreadData& data, std::size_t jobIndex, std::unique_lock<std::mutex>& lock);

    /**
     * @brief Get the thread data for the calling thread
     *
     * @return Pointer to the thread data for the current thread, or nullptr if not a pool thread
     */
    ThreadData* GetCallerThreadData() const noexcept;

    /**
     * @brief Create a new worker thread for the pool
     *
     * @return A new thread object that executes the thread pool worker function
     */
    std::thread MakeThread();

    /**
     * @brief Assign threads to a nested proxy
     *
     * @param proxy The proxy data to fill with thread assignments
     * @param maxCount Maximum number of threads to assign
     */
    void FillThreadsForNestedProxy(ProxyData* proxy, std::size_t maxCount);

    /**
     * @brief Get the next available thread ID for a proxy
     *
     * @return A unique thread ID value for the new proxy
     */
    std::size_t GetNextThreadId() noexcept;

    std::atomic<bool>                        Initialized{};
    std::atomic<bool>                        Joining{};
    std::vector<std::unique_ptr<ThreadData>> Threads;  // Thread pool, fixed size
    std::atomic<std::size_t>                 NextProxyThreadId{1};

public:
    /**
     * @brief Get the singleton instance of the thread pool
     *
     * @return Reference to the global thread pool instance
     */
    QUARISMA_API static thread_pool& GetInstance();
};

}  // namespace smp
}  // namespace detail
}  // namespace quarisma

#endif
/* QUARISMA-HeaderTest-Exclude: thread_pool.h */
