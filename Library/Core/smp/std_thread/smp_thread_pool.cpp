// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "smp/std_thread/smp_thread_pool.h"

#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <future>
#include <iostream>

#include "smp/common/smp_tools_impl.h"

namespace conductor
{
namespace detail
{
namespace smp
{

static constexpr std::size_t no_running_job = (std::numeric_limits<std::size_t>::max)();

struct smp_thread_pool::thread_job
{
    thread_job(proxy_data* proxy = nullptr, std::function<void()> function = nullptr)
        : m_proxy{proxy}, m_function{std::move(function)}
    {
    }

    proxy_data*           m_proxy{};
    std::function<void()> m_function;
    std::promise<void>    m_promise;
};

struct smp_thread_pool::thread_data
{
    std::vector<thread_job> m_jobs;
    std::size_t             m_running_job{no_running_job};
    std::thread             m_system_thread;
    std::mutex              m_mutex;
    std::condition_variable m_condition_variable;
};

struct smp_thread_pool::proxy_thread_data
{
    proxy_thread_data(thread_data* thread_data = nullptr, std::size_t id = 0)
        : m_thread{thread_data}, m_id{id}
    {
    }

    thread_data* m_thread{};
    std::size_t  m_id{};
};

struct smp_thread_pool::proxy_data
{
    smp_thread_pool*               m_pool{};
    proxy_data*                    m_parent{};
    std::vector<proxy_thread_data> m_threads;
    std::size_t                    m_next_thread{};
    std::vector<std::future<void>> m_jobs_futures;
    std::mutex                     m_mutex;
};

void smp_thread_pool::run_job(
    thread_data& data, std::size_t job_index, std::unique_lock<std::mutex>& lock)
{
    assert(lock.owns_lock() && "Caller must have locked mutex");
    assert(job_index < data.m_jobs.size() && "job_index out of range");

    const auto old_running_job = data.m_running_job;
    data.m_running_job         = job_index;
    auto function              = std::move(data.m_jobs[data.m_running_job].m_function);
    lock.unlock();

    try
    {
        function();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Function called by " << smp_thread_pool::get_instance().get_thread_id()
                  << " has thrown an exception. The exception is ignored. what():\n"
                  << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Function called by " << smp_thread_pool::get_instance().get_thread_id()
                  << " has thrown an unknown exception. The exception is ignored." << std::endl;
    }

    lock.lock();
    data.m_jobs[data.m_running_job].m_promise.set_value();
    data.m_jobs.erase(data.m_jobs.begin() + job_index);
    data.m_running_job = old_running_job;
}

smp_thread_pool::proxy::proxy(std::unique_ptr<proxy_data>&& data) : m_data{std::move(data)} {}

smp_thread_pool::proxy::~proxy()
{
    if (!m_data->m_jobs_futures.empty())
    {
        std::cerr << "Proxy not joined. Terminating." << std::endl;
        std::terminate();
    }
}

smp_thread_pool::proxy::proxy(proxy&&) noexcept                             = default;
smp_thread_pool::proxy& smp_thread_pool::proxy::operator=(proxy&&) noexcept = default;

void smp_thread_pool::proxy::join()
{
    if (this->is_top_level())
    {
        for (auto& future : m_data->m_jobs_futures)
        {
            future.wait();
        }
    }
    else
    {
        thread_data& thread_data_ref = *m_data->m_threads[0].m_thread;
        assert(thread_data_ref.m_system_thread.get_id() == std::this_thread::get_id());

        while (true)
        {
            std::unique_lock<std::mutex> lock{thread_data_ref.m_mutex};

            auto it = std::find_if(
                thread_data_ref.m_jobs.begin(),
                thread_data_ref.m_jobs.end(),
                [this](thread_job& job) { return job.m_proxy == m_data.get(); });

            if (it == thread_data_ref.m_jobs.end())
            {
                break;
            }

            const auto job_index =
                static_cast<std::size_t>(std::distance(thread_data_ref.m_jobs.begin(), it));
            run_job(thread_data_ref, job_index, lock);
        }

        for (auto& future : m_data->m_jobs_futures)
        {
            future.wait();
        }
    }

    m_data->m_jobs_futures.clear();
}

void smp_thread_pool::proxy::do_job(std::function<void()> job)
{
    m_data->m_next_thread = (m_data->m_next_thread + 1) % m_data->m_threads.size();
    auto& proxy_thread    = m_data->m_threads[m_data->m_next_thread];

    if (!this->is_top_level() && m_data->m_next_thread == 0)
    {
        assert(std::this_thread::get_id() == proxy_thread.m_thread->m_system_thread.get_id());

        std::unique_lock<std::mutex> lock{proxy_thread.m_thread->m_mutex};
        proxy_thread.m_thread->m_jobs.emplace_back(m_data.get(), std::move(job));
    }
    else
    {
        std::unique_lock<std::mutex> lock{proxy_thread.m_thread->m_mutex};

        auto& jobs = proxy_thread.m_thread->m_jobs;
        jobs.emplace_back(m_data.get(), std::move(job));
        m_data->m_jobs_futures.emplace_back(jobs.back().m_promise.get_future());

        lock.unlock();

        proxy_thread.m_thread->m_condition_variable.notify_one();
    }
}

std::vector<std::reference_wrapper<std::thread>> smp_thread_pool::proxy::get_threads() const
{
    std::vector<std::reference_wrapper<std::thread>> output;

    for (auto& proxy_thread : m_data->m_threads)
    {
        output.emplace_back(proxy_thread.m_thread->m_system_thread);
    }

    return output;
}

bool smp_thread_pool::proxy::is_top_level() const noexcept
{
    return m_data->m_parent == nullptr;
}

smp_thread_pool::smp_thread_pool()
{
    const std::size_t thread_count = static_cast<std::size_t>(
        smp_tools_impl<backend_type::std_thread>::get_estimated_default_number_of_threads());
    m_threads.reserve(thread_count);
    for (std::size_t i{}; i < thread_count; ++i)
    {
        std::unique_ptr<thread_data> data{new thread_data{}};
        data->m_system_thread = this->make_thread();
        m_threads.emplace_back(std::move(data));
    }

    m_initialized.store(true, std::memory_order_release);
}

smp_thread_pool::~smp_thread_pool()
{
    m_joining.store(true, std::memory_order_release);

    for (auto& thread_data_ptr : m_threads)
    {
        thread_data_ptr->m_condition_variable.notify_one();
    }

    for (auto& thread_data_ptr : m_threads)
    {
        thread_data_ptr->m_system_thread.join();
    }
}

smp_thread_pool::proxy smp_thread_pool::allocate_threads(std::size_t thread_count)
{
    if (thread_count == 0 || thread_count > this->thread_count())
    {
        thread_count = this->thread_count();
    }

    std::unique_ptr<proxy_data> proxy{new proxy_data{}};
    proxy->m_pool = this;
    proxy->m_threads.reserve(thread_count);

    thread_data* thread_data_ptr = this->get_caller_thread_data();
    if (thread_data_ptr)
    {
        proxy->m_parent = thread_data_ptr->m_jobs[thread_data_ptr->m_running_job].m_proxy;
        proxy->m_threads.emplace_back(thread_data_ptr, this->get_next_thread_id());
        this->fill_threads_for_nested_proxy(proxy.get(), thread_count);
    }
    else
    {
        proxy->m_parent = nullptr;
        for (std::size_t i{}; i < thread_count; ++i)
        {
            proxy->m_threads.emplace_back(m_threads[i].get(), this->get_next_thread_id());
        }
    }

    return smp_thread_pool::proxy{std::move(proxy)};
}

std::size_t smp_thread_pool::get_thread_id() const noexcept
{
    auto* thread_data_ptr = this->get_caller_thread_data();

    if (thread_data_ptr)
    {
        std::unique_lock<std::mutex> lock{thread_data_ptr->m_mutex};
        assert(thread_data_ptr->m_running_job != no_running_job && "Invalid state");
        auto& proxy_threads =
            thread_data_ptr->m_jobs[thread_data_ptr->m_running_job].m_proxy->m_threads;
        lock.unlock();

        for (const auto& proxy_thread : proxy_threads)
        {
            if (proxy_thread.m_thread == thread_data_ptr)
            {
                return proxy_thread.m_id;
            }
        }
    }

    return external_thread_id;
}

bool smp_thread_pool::is_parallel_scope() const noexcept
{
    return get_caller_thread_data() != nullptr;
}

bool smp_thread_pool::get_single_thread() const
{
    auto* thread_data_ptr = get_caller_thread_data();
    if (thread_data_ptr)
    {
        std::lock_guard<std::mutex> lock{thread_data_ptr->m_mutex};
        assert(thread_data_ptr->m_running_job != no_running_job && "Invalid state");
        return thread_data_ptr->m_jobs[thread_data_ptr->m_running_job]
                   .m_proxy->m_threads[0]
                   .m_thread == thread_data_ptr;
    }

    return false;
}

std::size_t smp_thread_pool::thread_count() const noexcept
{
    return m_threads.size();
}

smp_thread_pool::thread_data* smp_thread_pool::get_caller_thread_data() const noexcept
{
    for (const auto& thread_data_ptr : m_threads)
    {
        if (thread_data_ptr->m_system_thread.get_id() == std::this_thread::get_id())
        {
            return thread_data_ptr.get();
        }
    }

    return nullptr;
}

std::thread smp_thread_pool::make_thread()
{
    return std::thread{[this]()
                       {
                           while (!m_initialized.load(std::memory_order_acquire)) {}

                           thread_data& thread_data_ref = *this->get_caller_thread_data();

                           while (true)
                           {
                               std::unique_lock<std::mutex> lock{thread_data_ref.m_mutex};

                               thread_data_ref.m_condition_variable.wait(
                                   lock,
                                   [this, &thread_data_ref]
                                   {
                                       return !thread_data_ref.m_jobs.empty() ||
                                              m_joining.load(std::memory_order_acquire);
                                   });

                               if (thread_data_ref.m_jobs.empty())
                               {
                                   break;
                               }

                               run_job(thread_data_ref, thread_data_ref.m_jobs.size() - 1, lock);
                           }
                       }};
}

void smp_thread_pool::fill_threads_for_nested_proxy(proxy_data* proxy, std::size_t max_count)
{
    if (proxy->m_parent->m_threads.size() == m_threads.size())
    {
        return;
    }

    const auto is_free = [proxy](thread_data* thread_data_ptr)
    {
        for (auto* parent = proxy->m_parent; parent != nullptr; parent = parent->m_parent)
        {
            for (auto& proxy_thread : parent->m_threads)
            {
                if (proxy_thread.m_thread == thread_data_ptr)
                {
                    return false;
                }
            }
        }

        return true;
    };

    for (auto& thread_data_ptr : m_threads)
    {
        if (is_free(thread_data_ptr.get()))
        {
            proxy->m_threads.emplace_back(thread_data_ptr.get(), this->get_next_thread_id());
        }

        if (proxy->m_threads.size() == max_count)
        {
            break;
        }
    }
}

std::size_t smp_thread_pool::get_next_thread_id() noexcept
{
    return m_next_proxy_thread_id.fetch_add(1, std::memory_order_relaxed) + 1;
}

smp_thread_pool& smp_thread_pool::get_instance()
{
    static smp_thread_pool instance{};
    return instance;
}

}  // namespace smp
}  // namespace detail
}  // namespace conductor
