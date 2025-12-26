

// This file incorporates code from the Visualization Toolkit (VTK) and remains subject to the BSD-3-Clause VTK license.

#include "smp/STDThread/thread_pool.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <future>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace xsigma::detail::smp
{
static constexpr std::size_t NoRunningJob = (std::numeric_limits<std::size_t>::max)();

struct thread_pool::ThreadJob
{
    // This constructor is needed because aggregate initialization can not have default value
    // (prior to C++14)
    // also because emplace_back can not use aggregate initialization (prior to C++20)
    explicit ThreadJob(ProxyData* proxy = nullptr, std::function<void()> function = nullptr)
        : Proxy{proxy}, Function{std::move(function)}
    {
    }

    ProxyData*            Proxy{};   // Proxy that allocated this job
    std::function<void()> Function;  // Actual user job
    std::promise<void>    Promise;   // Set when job is done
};

struct thread_pool::ThreadData
{
    // stack of jobs, any thread can push, and only push, jobs (and Mutex must be locked)
    std::vector<ThreadJob> Jobs;
    // Current job (used to map thread to Proxy), using an index is okay as only this thread can
    // erase the job and other threads can only push back new jobs not insert. This constraint could
    // be relaxed by using unique ids instead.
    std::size_t             RunningJob{NoRunningJob};
    std::thread             SystemThread;       // the system thread, not really used
    std::mutex              Mutex;              // thread mutex, used for Jobs manipulation
    std::condition_variable ConditionVariable;  // thread cv, used to wake up the thread
};

struct thread_pool::ProxyThreadData
{
    // This constructor is needed because aggregate initialization can not have default value
    // (prior to C++14)
    // also because emplace_back can not use aggregate initialization (prior to C++20)
    explicit ProxyThreadData(ThreadData* threadData = nullptr, std::size_t id = 0)
        : Thread{threadData}, Id{id}
    {
    }

    ThreadData* Thread{};  // The thread data from the pool
    std::size_t Id{};      // Virtual thread ID, mainly used for thread local variables
};

struct thread_pool::ProxyData
{
    thread_pool*                   Pool{};        // Pool that created this proxy
    ProxyData*                     Parent{};      // either null (for top level) or the parent
    std::vector<ProxyThreadData>   Threads;       // Threads used by this
    std::size_t                    NextThread{};  // Round-robin thread for jobs
    std::vector<std::future<void>> JobsFutures;   // Used to know when job is done
    std::mutex                     Mutex;         // Used to synchronize
};

void thread_pool::RunJob(ThreadData& data, std::size_t jobIndex, std::unique_lock<std::mutex>& lock)
{
    assert(lock.owns_lock() && "Caller must have locked mutex");
    assert(jobIndex < data.Jobs.size() && "jobIndex out of range");

    const auto oldRunningJob = data.RunningJob;  // store old running job for nested threads
    data.RunningJob          = jobIndex;         // Set thread running job
    auto function            = std::move(data.Jobs[data.RunningJob].Function);
    lock.unlock();  // MSVC: warning C26110 is a false positive

    function();  // run the function

    lock.lock();
    data.Jobs[data.RunningJob].Promise.set_value();
    data.Jobs.erase(data.Jobs.begin() + jobIndex);  //NOLINT
    data.RunningJob = oldRunningJob;
}

thread_pool::Proxy::Proxy(std::unique_ptr<ProxyData>&& data) : Data{std::move(data)} {}

thread_pool::Proxy::~Proxy()
{
    if (!this->Data->JobsFutures.empty())
    {
        std::terminate();
    }
}

thread_pool::Proxy::Proxy(Proxy&&) noexcept                         = default;
thread_pool::Proxy& thread_pool::Proxy::operator=(Proxy&&) noexcept = default;

void thread_pool::Proxy::Join()
{
    if (this->IsTopLevel())  // wait for all futures, all jobs are done by other threads
    {
        for (auto& future : this->Data->JobsFutures)
        {
            future.wait();
        }
    }
    else  // nested run code in calling thread too
    {
        // Run jobs associated with this thread and proxy
        ThreadData& threadData = *this->Data->Threads[0].Thread;
        assert(threadData.SystemThread.get_id() == std::this_thread::get_id());

        while (true)
        {
            // protect access in case other thread push work for current thread
            std::unique_lock<std::mutex> lock{threadData.Mutex};

            auto it = std::find_if(
                threadData.Jobs.begin(),
                threadData.Jobs.end(),
                [this](const ThreadJob& job) { return job.Proxy == this->Data.get(); });

            if (it == threadData.Jobs.end())  // no remaining job associated to this proxy
            {
                break;
            }

            const auto jobIndex =
                static_cast<std::size_t>(std::distance(threadData.Jobs.begin(), it));
            RunJob(threadData, jobIndex, lock);
        }

        for (auto& future : this->Data->JobsFutures)
        {
            future.wait();
        }
    }

    this->Data->JobsFutures.clear();
}

void thread_pool::Proxy::DoJob(std::function<void()> job)
{
    this->Data->NextThread = (this->Data->NextThread + 1) % this->Data->Threads.size();
    auto& proxyThread      = this->Data->Threads[this->Data->NextThread];

    if (!this->IsTopLevel() &&
        this->Data->NextThread == 0)  // when nested, thread 0 is "this_thread"
    {
        assert(std::this_thread::get_id() == proxyThread.Thread->SystemThread.get_id());

        std::unique_lock<std::mutex> const lock{proxyThread.Thread->Mutex};
        proxyThread.Thread->Jobs.emplace_back(this->Data.get(), std::move(job));
    }
    else
    {
        std::unique_lock<std::mutex> lock{proxyThread.Thread->Mutex};

        auto& jobs = proxyThread.Thread->Jobs;
        jobs.emplace_back(this->Data.get(), std::move(job));
        this->Data->JobsFutures.emplace_back(jobs.back().Promise.get_future());

        lock.unlock();

        proxyThread.Thread->ConditionVariable.notify_one();
    }
}

std::vector<std::reference_wrapper<std::thread>> thread_pool::Proxy::GetThreads() const
{
    std::vector<std::reference_wrapper<std::thread>> output;

    for (auto& proxyThread : this->Data->Threads)
    {
        output.emplace_back(proxyThread.Thread->SystemThread);
    }

    return output;
}

bool thread_pool::Proxy::IsTopLevel() const noexcept
{
    return this->Data->Parent == nullptr;
}

thread_pool::thread_pool()
{
    const auto threadCount = static_cast<std::size_t>(std::thread::hardware_concurrency());

    this->Threads.reserve(threadCount);
    for (std::size_t i{}; i < threadCount; ++i)
    {
        std::unique_ptr<ThreadData> data{new ThreadData{}};
        data->SystemThread = this->MakeThread();
        this->Threads.emplace_back(std::move(data));
    }

    this->Initialized.store(true, std::memory_order_release);
}

thread_pool::~thread_pool()
{
    this->Joining.store(true, std::memory_order_release);

    for (const auto& threadData : this->Threads)
    {
        threadData->ConditionVariable.notify_one();
    }

    for (const auto& threadData : this->Threads)
    {
        threadData->SystemThread.join();
    }
}

thread_pool::Proxy thread_pool::AllocateThreads(std::size_t threadCount)
{
    if (threadCount == 0 || threadCount > this->ThreadCount())
    {
        threadCount = this->ThreadCount();
    }

    std::unique_ptr<ProxyData> proxy{new ProxyData{}};
    proxy->Pool = this;
    proxy->Threads.reserve(threadCount);

    // Check if we are in the pool
    ThreadData* threadData = this->GetCallerThreadData();
    if (threadData != nullptr)
    {
        // Don't lock since we are in the running job, in this thread
        proxy->Parent = threadData->Jobs[threadData->RunningJob].Proxy;
        // First thread is always current thread
        proxy->Threads.emplace_back(threadData, this->GetNextThreadId());
        this->FillThreadsForNestedProxy(proxy.get(), threadCount);
    }
    else
    {
        proxy->Parent = nullptr;
        for (std::size_t i{}; i < threadCount; ++i)
        {
            proxy->Threads.emplace_back(this->Threads[i].get(), this->GetNextThreadId());
        }
    }

    return Proxy{std::move(proxy)};
}

std::size_t thread_pool::GetThreadId() const
{
    auto* threadData = this->GetCallerThreadData();

    if (threadData != nullptr)
    {
        std::unique_lock<std::mutex> lock{threadData->Mutex};  // protect threadData->Jobs access
        assert(threadData->RunningJob != NoRunningJob && "Invalid state");
        const auto& proxyThreads = threadData->Jobs[threadData->RunningJob].Proxy->Threads;
        lock.unlock();

        auto proxy_it = std::find_if(
            proxyThreads.begin(),
            proxyThreads.end(),
            [threadData](const auto& proxyThread) { return proxyThread.Thread == threadData; });

        if (proxy_it != proxyThreads.end())
        {
            return proxy_it->Id;
        }
    }

    // Use 1 for any thread outside the pool and 2+ for ids of proxy thread because thread local
    // implementation uses ID "0" for invalid state
    return ExternalThreadID;
}

bool thread_pool::IsParallelScope() const noexcept
{
    return GetCallerThreadData() != nullptr;
}

bool thread_pool::GetSingleThread() const
{
    // Return true if the caller is the thread[0] of the current running proxy

    auto* threadData = GetCallerThreadData();
    if (threadData != nullptr)
    {
        std::scoped_lock const lock{threadData->Mutex};
        assert(threadData->RunningJob != NoRunningJob && "Invalid state");
        return threadData->Jobs[threadData->RunningJob].Proxy->Threads[0].Thread == threadData;
    }

    return false;
}

std::size_t thread_pool::ThreadCount() const noexcept
{
    return this->Threads.size();
}

thread_pool::ThreadData* thread_pool::GetCallerThreadData() const noexcept
{
    for (const auto& threadData : this->Threads)
    {
        if (threadData->SystemThread.get_id() == std::this_thread::get_id())
        {
            return threadData.get();
        }
    }

    return nullptr;
}

std::thread thread_pool::MakeThread()
{
    return std::thread{[this]()
                       {
                           while (!this->Initialized.load(std::memory_order_acquire)) {}

                           ThreadData& threadData = *this->GetCallerThreadData();

                           // Main loop for threads of the pool
                           // When they are woke up, they check for new job and stop if "this->Joining" is true
                           // and no more jobs are running
                           while (true)
                           {
                               std::unique_lock<std::mutex> lock{threadData.Mutex};

                               // Job stealing could be implemented but it will requires some changes in the process
                               // A thread that as no longer work to do could look at other threads jobs to "steal" a job
                               // from them and thus increase parallelism. This must take care of not generating deadlocks
                               // and should not increase Proxy parallelism above requested thread count.
                               // This goes out of the scope of current implementation.
                               threadData.ConditionVariable.wait(
                                   lock,
                                   [this, &threadData]
                                   {
                                       return !threadData.Jobs.empty() ||
                                              this->Joining.load(std::memory_order_acquire);
                                   });

                               if (threadData.Jobs.empty())
                               {
                                   break;  // joining
                               }

                               RunJob(threadData, threadData.Jobs.size() - 1, lock);
                           }
                       }};
}

void thread_pool::FillThreadsForNestedProxy(ProxyData* proxy, std::size_t maxCount)
{
    // This function assigns thread for proxies, this function assumes that the calling thread is
    // already part of the assigned thread for the proxy.
    // Otherwise it will assign thread pool threads that are not already used by any of proxy parents

    if (proxy->Parent->Threads.size() == this->Threads.size())
    {
        return;  // No thread will be available
    }

    const auto isFree = [proxy](ThreadData* threadData)
    {
        for (auto* parent = proxy->Parent; parent != nullptr; parent = parent->Parent)
        {
            bool const found = std::any_of(
                parent->Threads.begin(),
                parent->Threads.end(),
                [threadData](const auto& proxyThread) { return proxyThread.Thread == threadData; });
            if (found)
            {
                return false;
            }
        }

        return true;
    };

    for (auto& threadData : this->Threads)
    {
        if (isFree(threadData.get()))
        {
            proxy->Threads.emplace_back(threadData.get(), this->GetNextThreadId());
        }

        if (proxy->Threads.size() == maxCount)
        {
            break;
        }
    }
}

std::size_t thread_pool::GetNextThreadId() noexcept
{
    return this->NextProxyThreadId.fetch_add(1, std::memory_order_relaxed) + 1;
}

thread_pool& thread_pool::GetInstance()
{
    static thread_pool instance{};
    return instance;
}
}  // namespace xsigma::detail::smp
