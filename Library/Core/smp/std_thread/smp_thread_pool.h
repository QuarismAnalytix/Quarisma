// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class smp_thread_pool
 * @brief A thread pool implementation using std::thread
 *
 * smp_thread_pool class creates a thread pool of std::thread, the number
 * of thread must be specified at the initialization of the class.
 * The do_job() method is used attributes the job to a free thread, if all
 * threads are working, the job is kept in a queue. Note that smp_thread_pool
 * destructor joins threads and finish the jobs in the queue.
 */

#ifndef SMP_THREAD_POOL_H
#define SMP_THREAD_POOL_H

#include <atomic>      // For std::atomic
#include <functional>  // For std::function
#include <mutex>       // For std::unique_lock
#include <thread>      // For std::thread
#include <vector>      // For std::vector

#include "common/export.h"

namespace xsigma
{
namespace detail
{
namespace smp
{

/**
 * @brief Internal thread pool implementation used in SMP functions
 *
 * This class is designed to be a Singleton thread pool, but local pool can be allocated too.
 * This thread pool use a Proxy system that is used to allocate a certain amount of threads from
 * the pool, which enable support for SMP local scopes.
 * You need to have a Proxy to submit job to the pool.
 */
class XSIGMA_VISIBILITY smp_thread_pool
{
    // Internal data structures
    struct thread_job;
    struct thread_data;
    struct proxy_thread_data;
    struct proxy_data;

public:
    /**
   * @brief Proxy class used to submit work to the thread pool.
   */
    class XSIGMA_VISIBILITY proxy final
    {
    public:
        XSIGMA_API ~proxy();
        proxy(const proxy&)                       = delete;
        proxy&            operator=(const proxy&) = delete;
        XSIGMA_API        proxy(proxy&&) noexcept;
        XSIGMA_API proxy& operator=(proxy&&) noexcept;

        /**
     * @brief Blocks calling thread until all jobs are done.
     */
        XSIGMA_API void join();

        /**
     * @brief Add a job to the thread pool queue
     */
        XSIGMA_API void do_job(std::function<void()> job);

        /**
     * @brief Get a reference on all system threads used by this proxy
     */
        XSIGMA_API std::vector<std::reference_wrapper<std::thread>> get_threads() const;

        /**
     * @brief Return true if this proxy is allocated from a thread that does not belong to the pool
     */
        XSIGMA_API bool is_top_level() const noexcept;

    private:
        friend class smp_thread_pool;

        proxy(std::unique_ptr<proxy_data>&& data);

        std::unique_ptr<proxy_data> m_data;
    };

    XSIGMA_API smp_thread_pool();
    XSIGMA_API ~smp_thread_pool();
    smp_thread_pool(const smp_thread_pool&)            = delete;
    smp_thread_pool& operator=(const smp_thread_pool&) = delete;

    /**
   * @brief Create a proxy
   */
    XSIGMA_API proxy allocate_threads(std::size_t thread_count = 0);

    /**
   * Value returned by `get_thread_id` when called by a thread that does not belong to the pool.
   */
    static constexpr std::size_t external_thread_id = 1;

    /**
   * @brief Get caller proxy thread virtual ID
   */
    XSIGMA_API std::size_t get_thread_id() const noexcept;

    /**
   * @brief Returns true when called from a proxy thread, false otherwise.
   */
    XSIGMA_API bool is_parallel_scope() const noexcept;

    /**
   * @brief Returns true for a single proxy thread, false for the others.
   */
    XSIGMA_API bool single_thread() const;

    /**
   * @brief Returns number of system thread used by the thread pool.
   */
    XSIGMA_API std::size_t thread_count() const noexcept;

    XSIGMA_API static smp_thread_pool& instance();

private:
    static void run_job(
        thread_data& data, std::size_t job_index, std::unique_lock<std::mutex>& lock);

    thread_data* get_caller_thread_data() const noexcept;

    std::thread make_thread();
    void        fill_threads_for_nested_proxy(proxy_data* proxy, std::size_t max_count);
    std::size_t get_next_thread_id() noexcept;

    std::atomic<bool>                         m_initialized{};
    std::atomic<bool>                         m_joining{};
    std::vector<std::unique_ptr<thread_data>> m_threads;  // Thread pool, fixed size
    std::atomic<std::size_t>                  m_next_proxy_thread_id{1};
};

}  // namespace smp
}  // namespace detail
}  // namespace xsigma

#endif
