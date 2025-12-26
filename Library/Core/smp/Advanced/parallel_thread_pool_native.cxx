#include "smp/Advanced/parallel_thread_pool_native.h"

#include <atomic>
#include <functional>
#include <memory>
#include <utility>

#include "smp/Advanced/thread_pool.h"
#include "util/exception.h"

namespace xsigma::detail::smp::Advanced
{
namespace
{
constexpr int kNotSet   = -1;
constexpr int kConsumed = -2;

std::atomic<int> g_num_interop_threads{kNotSet};

task_thread_pool_base& GetInteropPool()
{
    static std::shared_ptr<task_thread_pool_base> const pool =
        create_task_thread_pool(g_num_interop_threads.exchange(kConsumed));
    return *pool;
}
}  // namespace

void set_num_interop_threads(int nthreads)
{
    XSIGMA_CHECK(nthreads > 0, "Expected positive number of threads");

    int expected = kNotSet;
    XSIGMA_CHECK(
        g_num_interop_threads.compare_exchange_strong(expected, nthreads),
        "Cannot set number of interop threads after parallel work has started");
}

size_t get_num_interop_threads()
{
    int const nthreads = g_num_interop_threads.load();
    if (nthreads > 0)
    {
        return static_cast<size_t>(nthreads);
    }
    if (nthreads == kNotSet)
    {
        return task_thread_pool_base::DefaultNumThreads();
    }
    return GetInteropPool().Size();
}

void launch_no_thread_state(std::function<void()> fn)
{
    GetInteropPool().Run(std::move(fn));
}

void launch(std::function<void()> func)
{
    launch_no_thread_state(std::move(func));
}

}  // namespace xsigma::detail::smp::Advanced
